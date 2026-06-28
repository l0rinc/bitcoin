// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <common/url.h>
#include <netaddress.h>
#include <rpc/protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/net.h>
#include <test/fuzz/util.h>
#include <test/util/time.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


std::string_view RequestMethodString(HTTPRequestMethod m);

namespace {
std::optional<std::string> ReferenceQueryParameterFromUri(const std::string_view uri, const std::string_view key)
{
    const size_t start{uri.find('?')};
    if (start == std::string::npos) return std::nullopt;
    size_t end{uri.find('#', start)};
    if (end == std::string::npos) end = uri.size();

    size_t pos{start + 1};
    while (pos <= end) {
        size_t next{uri.find('&', pos)};
        if (next == std::string::npos || next > end) next = end;
        const std::string_view param{uri.data() + pos, next - pos};
        const size_t delim{param.find('=')};
        if (UrlDecode(param.substr(0, delim)) == key) {
            if (delim == std::string::npos) return "";
            return UrlDecode(param.substr(delim + 1));
        }
        if (next == end) break;
        pos = next + 1;
    }
    return std::nullopt;
}

std::vector<std::string> QueryKeysForOracle(const std::string_view uri, FuzzedDataProvider& fuzzed_data_provider)
{
    std::vector<std::string> keys{
        "",
        "count",
        "p",
        "p1",
        "verbose",
        fuzzed_data_provider.ConsumeRandomLengthString(16),
    };

    const size_t start{uri.find('?')};
    if (start == std::string::npos) return keys;
    size_t end{uri.find('#', start)};
    if (end == std::string::npos) end = uri.size();

    size_t pos{start + 1};
    for (unsigned params{0}; pos <= end && params < 16; ++params) {
        size_t next{uri.find('&', pos)};
        if (next == std::string::npos || next > end) next = end;
        const std::string_view param{uri.data() + pos, next - pos};
        const size_t delim{param.find('=')};
        keys.push_back(UrlDecode(param.substr(0, delim)));
        if (delim != std::string::npos) keys.emplace_back(param.substr(0, delim));
        if (next == end) break;
        pos = next + 1;
    }
    return keys;
}

std::optional<std::string> ReferenceHeaderFindFirst(const std::vector<std::pair<std::string, std::string>>& headers, const std::string_view key)
{
    for (const auto& [header_key, header_value] : headers) {
        if (CaseInsensitiveEqual(key, header_key)) return header_value;
    }
    return std::nullopt;
}

std::vector<std::string> ReferenceHeaderFindAll(const std::vector<std::pair<std::string, std::string>>& headers, const std::string_view key)
{
    std::vector<std::string> ret;
    for (const auto& [header_key, header_value] : headers) {
        if (CaseInsensitiveEqual(key, header_key)) ret.push_back(header_value);
    }
    return ret;
}

std::string ReferenceHeaderStringify(const std::vector<std::pair<std::string, std::string>>& headers)
{
    std::string ret;
    for (const auto& [header_key, header_value] : headers) {
        ret += header_key + ": " + header_value + "\r\n";
    }
    ret += "\r\n";
    return ret;
}

void ReferenceHeaderRemoveAll(std::vector<std::pair<std::string, std::string>>& headers, const std::string_view key)
{
    for (auto it{headers.begin()}; it != headers.end();) {
        if (CaseInsensitiveEqual(key, it->first)) {
            it = headers.erase(it);
        } else {
            ++it;
        }
    }
}

void AssertHeaderCollectionContracts(FuzzedDataProvider& fuzzed_data_provider)
{
    http_bitcoin::HTTPHeaders headers;
    std::vector<std::pair<std::string, std::string>> reference_headers;

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 32)
    {
        const std::string key{fuzzed_data_provider.ConsumeRandomLengthString(16)};
        const std::string value{fuzzed_data_provider.ConsumeRandomLengthString(16)};
        const uint8_t op{fuzzed_data_provider.ConsumeIntegralInRange<uint8_t>(0, 2)};
        if (op == 0) {
            headers.Write(std::string{key}, std::string{value});
            reference_headers.emplace_back(key, value);
        } else if (op == 1) {
            const std::string before{headers.Stringify()};
            const bool existed{ReferenceHeaderFindFirst(reference_headers, key).has_value()};
            headers.RemoveAll(key);
            ReferenceHeaderRemoveAll(reference_headers, key);
            assert(existed || headers.Stringify() == before);
        }

        const std::string query_key{fuzzed_data_provider.ConsumeBool() && !reference_headers.empty() ?
            PickValue(fuzzed_data_provider, reference_headers).first :
            fuzzed_data_provider.ConsumeRandomLengthString(16)};
        const auto first{headers.FindFirst(query_key)};
        const auto expected_first{ReferenceHeaderFindFirst(reference_headers, query_key)};
        assert(first == expected_first);

        const auto all{headers.FindAll(query_key)};
        const auto expected_all{ReferenceHeaderFindAll(reference_headers, query_key)};
        const std::vector<std::string> all_values{all.begin(), all.end()};
        assert(all_values == expected_all);
        if (!all.empty()) assert(first == std::optional<std::string>{std::string{all.front()}});
        assert(headers.Stringify() == ReferenceHeaderStringify(reference_headers));
    }
}

void AssertWriteReplyContracts(http_bitcoin::HTTPRequest& http_request, FuzzedDataProvider& fuzzed_data_provider, FakeSteadyClock& clock)
{
    using http_bitcoin::HTTPRemoteClient;

    std::vector<HTTPStatusCode> statuses{
        HTTP_OK,
        HTTP_NO_CONTENT,
        HTTP_BAD_REQUEST,
        HTTP_UNAUTHORIZED,
        HTTP_FORBIDDEN,
        HTTP_NOT_FOUND,
        HTTP_BAD_METHOD,
        HTTP_CONTENT_TOO_LARGE,
        HTTP_INTERNAL_SERVER_ERROR,
        HTTP_SERVICE_UNAVAILABLE};
    const HTTPStatusCode status{PickValue(fuzzed_data_provider, statuses)};
    const bool needs_body{status != HTTP_NO_CONTENT};
    const std::string reply_body{needs_body ? fuzzed_data_provider.ConsumeRandomLengthString(32) : std::string{}};

    auto client{std::make_shared<HTTPRemoteClient>(
        /*id=*/0,
        CService{},
        std::make_unique<FuzzedSock>(fuzzed_data_provider, clock))};
    http_request.m_client = client;
    client->m_req_busy = true;

    constexpr std::byte queued_byte{0x42};
    {
        LOCK(client->m_send_mutex);
        client->m_send_buffer.push_back(queued_byte);
    }

    const auto connection_header{http_request.GetHeader("Connection")};
    const bool request_keep_alive{connection_header.first && ToLower(connection_header.second) == "keep-alive"};
    const bool request_close{connection_header.first && ToLower(connection_header.second) == "close"};

    bool expected_keep_alive{false};
    bool expected_content_length{false};
    if (http_request.m_version.major == 1 && http_request.m_version.minor == 0) {
        expected_keep_alive = request_keep_alive;
        expected_content_length = needs_body && request_keep_alive;
    } else if (http_request.m_version.major == 1 && http_request.m_version.minor >= 1) {
        expected_keep_alive = true;
        expected_content_length = needs_body;
    }
    if (request_close) expected_keep_alive = false;

    http_request.WriteReply(status, reply_body);

    const std::vector<std::byte> sent{WITH_LOCK(client->m_send_mutex, return client->m_send_buffer)};
    assert(sent.size() > 1);
    assert(sent.front() == queued_byte);

    std::string response;
    response.reserve(sent.size() - 1);
    for (auto it{sent.begin() + 1}; it != sent.end(); ++it) {
        response.push_back(static_cast<char>(std::to_integer<unsigned char>(*it)));
    }

    const std::string status_line{
        "HTTP/" + std::to_string(http_request.m_version.major) +
        "." + std::to_string(http_request.m_version.minor) +
        " " + std::to_string(status) +
        " " + std::string{HTTPStatusReasonString(status)} + "\r\n"};
    assert(response.starts_with(status_line));

    const size_t header_end{response.find("\r\n\r\n")};
    assert(header_end != std::string::npos);
    const std::string response_headers{response.substr(0, header_end + 4)};
    assert(response.substr(header_end + 4) == reply_body);

    const std::string content_length_header{"Content-Length: " + std::to_string(reply_body.size()) + "\r\n"};
    if (expected_content_length) {
        assert(response_headers.find(content_length_header) != std::string::npos);
    } else {
        assert(response_headers.find("Content-Length: ") == std::string::npos);
    }
    if (needs_body) {
        assert(response_headers.find("Content-Type: text/html; charset=ISO-8859-1\r\n") != std::string::npos);
    } else {
        assert(response_headers.find("Content-Type: ") == std::string::npos);
    }
    if (request_close) {
        assert(response_headers.find("Connection: close\r\n") != std::string::npos);
    } else if (http_request.m_version.minor == 0 && request_keep_alive) {
        assert(response_headers.find("Connection: keep-alive\r\n") != std::string::npos);
    }

    assert(client->m_keep_alive.load() == expected_keep_alive);
    assert(WITH_LOCK(client->m_send_mutex, return client->m_send_ready));
    assert(!client->m_req_busy.load());
}
} // namespace

FUZZ_TARGET(http_request)
{
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::MAX_BODY_SIZE;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    SetMockTime(1);
    FakeSteadyClock steady_clock;
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    AssertHeaderCollectionContracts(fuzzed_data_provider);
    const std::vector<std::byte> http_buffer{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 4096)};

    HTTPRequest http_request;
    LineReader reader(http_buffer, MAX_HEADERS_SIZE);
    try {
        if (!http_request.LoadControlData(reader)) return;
        if (!http_request.LoadHeaders(reader)) return;
        if (!http_request.LoadBody(reader)) return;
    } catch (const std::runtime_error&) {
        return;
    }

    const HTTPRequestMethod request_method = http_request.GetRequestMethod();
    (void)RequestMethodString(request_method);
    (void)http_request.GetURI();
    (void)http_request.GetHeader("Host");
    for (const std::string& key : QueryKeysForOracle(http_request.GetURI(), fuzzed_data_provider)) {
        assert(http_request.GetQueryParameter(key) == ReferenceQueryParameterFromUri(http_request.GetURI(), key));
    }
    AssertWriteReplyContracts(http_request, fuzzed_data_provider, steady_clock);
    std::string header = fuzzed_data_provider.ConsumeRandomLengthString(16);
    const auto request_header_before{http_request.GetHeader(header)};
    (void)http_request.WriteHeader(std::string(header), fuzzed_data_provider.ConsumeRandomLengthString(16));
    assert(http_request.GetHeader(header) == request_header_before);
    const std::string body = http_request.ReadBody();

    const auto transfer_encoding{http_request.GetHeader("Transfer-Encoding")};
    const bool chunked{transfer_encoding.first && ToLower(transfer_encoding.second) == "chunked"};
    const auto content_lengths{http_request.m_headers.FindAll("Content-Length")};
    if (chunked) {
        assert(body.size() <= MAX_BODY_SIZE);
    } else if (content_lengths.empty()) {
        assert(body.empty());
    } else {
        const auto content_length{ToIntegral<uint64_t>(content_lengths.front())};
        assert(content_length);
        assert(body.size() == *content_length);
        assert(body.size() <= MAX_BODY_SIZE);
    }
}
