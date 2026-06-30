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

#include <array>
#include <cassert>
#include <cerrno>
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
    const bool response_has_body{status != HTTP_NO_CONTENT};
    const bool needs_body_headers{response_has_body && http_request.GetRequestMethod() != HTTPRequestMethod::HEAD};
    const std::string reply_body{response_has_body ? fuzzed_data_provider.ConsumeRandomLengthString(32) : std::string{}};
    const bool optimistic_send{fuzzed_data_provider.ConsumeBool()};
    const bool response_close{fuzzed_data_provider.ConsumeBool()};
    const bool response_content_type{fuzzed_data_provider.ConsumeBool()};
    std::vector<std::string> response_content_type_values{"text/plain", "application/json", "application/octet-stream"};
    const std::string response_content_type_value{response_content_type ?
        PickValue(fuzzed_data_provider, response_content_type_values) :
        std::string{}};

    std::shared_ptr<DynSock::Pipes> pipes;
    std::unique_ptr<Sock> sock;
    if (optimistic_send) {
        pipes = std::make_shared<DynSock::Pipes>();
        sock = std::make_unique<DynSock>(pipes);
    } else {
        sock = std::make_unique<FuzzedSock>(fuzzed_data_provider, clock);
    }
    auto client{std::make_shared<HTTPRemoteClient>(
        /*id=*/0,
        CService{},
        std::move(sock))};
    http_request.m_client = client;
    client->m_req_busy = true;

    constexpr std::byte queued_byte{0x42};
    if (!optimistic_send) {
        LOCK(client->m_send_mutex);
        client->m_send_buffer.push_back(queued_byte);
    }

    const auto connection_header{http_request.GetHeader("Connection")};
    const bool request_keep_alive{connection_header.first && ToLower(connection_header.second) == "keep-alive"};
    const bool request_close{connection_header.first && ToLower(connection_header.second) == "close"};

    if (response_close) {
        http_request.WriteHeader("Connection", "close");
    }
    if (response_content_type) {
        http_request.WriteHeader("Content-Type", std::string{response_content_type_value});
    }

    bool expected_keep_alive{false};
    bool expected_content_length{false};
    if (http_request.m_version.major == 1 && http_request.m_version.minor == 0) {
        expected_keep_alive = request_keep_alive && !response_close;
        expected_content_length = needs_body_headers && request_keep_alive;
    } else if (http_request.m_version.major == 1 && http_request.m_version.minor >= 1) {
        expected_keep_alive = !response_close;
        expected_content_length = needs_body_headers;
    }
    if (request_close) expected_keep_alive = false;
    if (response_close) expected_keep_alive = false;
    if (response_close && http_request.m_version.major == 1 && http_request.m_version.minor == 0) {
        expected_content_length = false;
    }

    http_request.WriteReply(status, reply_body);

    std::string response;
    if (optimistic_send) {
        const std::vector<std::byte> buffered{WITH_LOCK(client->m_send_mutex, return client->m_send_buffer)};
        assert(buffered.empty());
        assert(WITH_LOCK(client->m_send_mutex, return !client->m_send_ready));
        assert(!client->m_connection_busy.load());
        assert(client->m_disconnect.load() == !expected_keep_alive);

        WITH_LOCK(client->m_sock_mutex, client->m_sock.reset());
        std::array<char, 4096> sent{};
        const ssize_t sent_size{pipes->send.GetBytes(sent.data(), sent.size())};
        assert(sent_size > 0);
        response.assign(sent.data(), sent.data() + sent_size);
    } else {
        const std::vector<std::byte> sent{WITH_LOCK(client->m_send_mutex, return client->m_send_buffer)};
        assert(sent.size() > 1);
        assert(sent.front() == queued_byte);
        assert(WITH_LOCK(client->m_send_mutex, return client->m_send_ready));
        assert(client->m_connection_busy.load());
        assert(!client->m_disconnect.load());

        response.reserve(sent.size() - 1);
        for (auto it{sent.begin() + 1}; it != sent.end(); ++it) {
            response.push_back(static_cast<char>(std::to_integer<unsigned char>(*it)));
        }
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
    if (response_content_type) {
        assert(response_headers.find("Content-Type: " + response_content_type_value + "\r\n") != std::string::npos);
    } else if (needs_body_headers) {
        assert(response_headers.find("Content-Type: text/html; charset=ISO-8859-1\r\n") != std::string::npos);
    } else {
        assert(response_headers.find("Content-Type: ") == std::string::npos);
    }
    if (request_close || response_close) {
        assert(response_headers.find("Connection: close\r\n") != std::string::npos);
    } else if (http_request.m_version.minor == 0 && request_keep_alive) {
        assert(response_headers.find("Connection: keep-alive\r\n") != std::string::npos);
    }

    assert(client->m_keep_alive.load() == expected_keep_alive);
    assert(!client->m_req_busy.load());
}

void AssertReadRequestContracts(const std::vector<std::byte>& http_buffer)
{
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::HTTPRemoteClient;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    HTTPRequest direct_request;
    LineReader direct_reader(http_buffer, MAX_HEADERS_SIZE);

    bool direct_complete{false};
    bool direct_threw{false};
    try {
        direct_complete = direct_request.LoadControlData(direct_reader) &&
                          direct_request.LoadHeaders(direct_reader) &&
                          direct_request.LoadBody(direct_reader);
    } catch (const std::runtime_error&) {
        direct_threw = true;
    }

    auto client{std::make_shared<HTTPRemoteClient>(
        /*id=*/1,
        CService{},
        std::make_unique<StaticContentsSock>(""))};
    client->m_recv_buffer = http_buffer;
    const std::vector<std::byte> recv_buffer_before{client->m_recv_buffer};

    HTTPRequest client_request{client};
    if (direct_threw) {
        try {
            (void)client->ReadRequest(client_request);
            assert(false);
        } catch (const std::runtime_error&) {
        }
        assert(client->m_recv_buffer == recv_buffer_before);
        return;
    }

    const bool client_complete{client->ReadRequest(client_request)};
    assert(client_complete == direct_complete);

    if (!client_complete) {
        assert(client->m_recv_buffer == recv_buffer_before);
        return;
    }

    assert(direct_reader.Consumed() <= recv_buffer_before.size());
    const std::vector<std::byte> expected_remaining{
        recv_buffer_before.begin() + direct_reader.Consumed(),
        recv_buffer_before.end()};
    assert(client->m_recv_buffer == expected_remaining);

    assert(client_request.GetRequestMethod() == direct_request.GetRequestMethod());
    assert(client_request.GetURI() == direct_request.GetURI());
    assert(client_request.m_version.major == direct_request.m_version.major);
    assert(client_request.m_version.minor == direct_request.m_version.minor);
    assert(client_request.m_headers.Stringify() == direct_request.m_headers.Stringify());
    assert(client_request.ReadBody() == direct_request.ReadBody());
}

class ScriptedSendSock final : public ZeroSock
{
public:
    ScriptedSendSock(ssize_t result, int err) : m_result{result}, m_errno{err} {}

    ssize_t Send(const void* data, size_t len, int flags) const override
    {
        m_send_called = true;
        m_flags = flags;
        const auto* bytes{static_cast<const std::byte*>(data)};
        m_attempted.assign(bytes, bytes + len);
        if (m_result < 0) {
            errno = m_errno;
            return -1;
        }
        assert(static_cast<size_t>(m_result) <= len);
        return m_result;
    }

    const ssize_t m_result;
    const int m_errno;
    mutable bool m_send_called{false};
    mutable int m_flags{0};
    mutable std::vector<std::byte> m_attempted;
};

void AssertSendBufferContracts(FuzzedDataProvider& fuzzed_data_provider)
{
    using http_bitcoin::HTTPRemoteClient;

    std::vector<std::byte> send_buffer;
    if (fuzzed_data_provider.ConsumeBool()) {
        send_buffer = ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 256);
        if (send_buffer.empty()) send_buffer.push_back(std::byte{0});
    }

    constexpr int send_errnos[]{
        EAGAIN,
        EINTR,
        EWOULDBLOCK,
        EINPROGRESS,
        ECONNRESET,
        EPIPE,
        EBADF,
        EINVAL,
    };
    const bool send_error{fuzzed_data_provider.ConsumeBool()};
    const int send_errno{fuzzed_data_provider.PickValueInArray(send_errnos)};
    const ssize_t send_result{send_error ? -1 : fuzzed_data_provider.ConsumeIntegralInRange<ssize_t>(0, static_cast<ssize_t>(send_buffer.size()))};
    const bool keep_alive{fuzzed_data_provider.ConsumeBool()};

    auto sock{std::make_unique<ScriptedSendSock>(send_result, send_errno)};
    const auto* scripted_sock{sock.get()};
    auto client{std::make_shared<HTTPRemoteClient>(
        /*id=*/2,
        CService{},
        std::move(sock))};
    {
        LOCK(client->m_send_mutex);
        client->m_send_buffer = send_buffer;
    }
    client->m_keep_alive = keep_alive;

    const bool initial_send_ready{fuzzed_data_provider.ConsumeBool()};
    const bool initial_connection_busy{fuzzed_data_provider.ConsumeBool()};
    {
        LOCK(client->m_send_mutex);
        client->m_send_ready = initial_send_ready;
    }
    client->m_connection_busy = initial_connection_busy;
    client->m_disconnect = false;

    const bool ret{client->MaybeSendBytesFromBuffer()};

    const std::vector<std::byte> remaining{WITH_LOCK(client->m_send_mutex, return client->m_send_buffer)};
    if (send_buffer.empty()) {
        assert(ret);
        assert(!scripted_sock->m_send_called);
        assert(remaining.empty());
        assert(WITH_LOCK(client->m_send_mutex, return client->m_send_ready) == initial_send_ready);
        assert(client->m_connection_busy.load() == initial_connection_busy);
        assert(!client->m_disconnect.load());
        return;
    }

    assert(scripted_sock->m_send_called);
    assert(scripted_sock->m_attempted == send_buffer);
    assert((scripted_sock->m_flags & MSG_NOSIGNAL) == MSG_NOSIGNAL);
    assert((scripted_sock->m_flags & MSG_DONTWAIT) == MSG_DONTWAIT);
#ifdef MSG_MORE
    assert(!(scripted_sock->m_flags & MSG_MORE));
#endif

    if (send_result < 0) {
        assert(remaining == send_buffer);
        if (IOErrorIsPermanent(send_errno)) {
            assert(!ret);
            assert(WITH_LOCK(client->m_send_mutex, return !client->m_send_ready));
            assert(client->m_disconnect.load());
        } else {
            assert(ret);
            assert(WITH_LOCK(client->m_send_mutex, return client->m_send_ready));
            assert(client->m_connection_busy.load());
            assert(!client->m_disconnect.load());
        }
        return;
    }

    const auto sent_size{static_cast<size_t>(send_result)};
    const std::vector<std::byte> expected_remaining{send_buffer.begin() + sent_size, send_buffer.end()};
    assert(remaining == expected_remaining);
    if (remaining.empty()) {
        assert(WITH_LOCK(client->m_send_mutex, return !client->m_send_ready));
        assert(!client->m_connection_busy.load());
        assert(client->m_disconnect.load() == !keep_alive);
        assert(ret == keep_alive);
    } else {
        assert(ret);
        assert(WITH_LOCK(client->m_send_mutex, return client->m_send_ready));
        assert(client->m_connection_busy.load());
        assert(!client->m_disconnect.load());
    }
}
} // namespace

FUZZ_TARGET(http_request)
{
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::MAX_BODY_SIZE;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    ResetFuzzedSockMockedFds();
    SetMockTime(1);
    FakeSteadyClock steady_clock;
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    AssertHeaderCollectionContracts(fuzzed_data_provider);
    AssertSendBufferContracts(fuzzed_data_provider);
    const std::vector<std::byte> http_buffer{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 4096)};
    AssertReadRequestContracts(http_buffer);

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
