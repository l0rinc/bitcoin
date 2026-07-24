// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <httpserver.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/signalinterrupt.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <stdexcept>
#include <vector>

namespace {

struct ParsedRequest {
    HTTPRequestMethod method;
    http_bitcoin::HTTPVersion version;
    std::string target;
    std::string headers;
    std::string body;
    size_t consumed;
};

void AssertBodyContract(const http_bitcoin::HTTPRequest& request,
                        const util::LineReader& reader,
                        size_t body_start,
                        size_t buffer_size)
{
    assert(reader.Consumed() <= buffer_size);

    const auto [has_transfer_encoding, transfer_encoding] = request.GetHeader("Transfer-Encoding");
    const bool chunked{has_transfer_encoding && ToLower(transfer_encoding) == "chunked"};
    const auto [has_content_length, content_length_string] = request.GetHeader("Content-Length");

    if (chunked) {
        assert(request.m_body.size() <= http_bitcoin::MAX_BODY_SIZE);
        return;
    }
    if (!has_content_length) {
        assert(request.m_body.empty());
        assert(reader.Consumed() == body_start);
        return;
    }

    const auto content_length{ToIntegral<uint64_t>(content_length_string)};
    assert(content_length);
    assert(request.m_body.size() == *content_length);
    assert(reader.Consumed() == body_start + *content_length);
}

std::optional<ParsedRequest> ParseRequest(std::span<const std::byte> input)
{
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    HTTPRequest request;
    LineReader reader(input, MAX_HEADERS_SIZE);
    try {
        if (!request.LoadControlData(reader)) return std::nullopt;
        if (!request.LoadHeaders(reader)) return std::nullopt;
        const size_t body_start{reader.Consumed()};
        if (!request.LoadBody(reader)) return std::nullopt;
        AssertBodyContract(request, reader, body_start, input.size());
    } catch (const std::runtime_error&) {
        return std::nullopt;
    }

    return ParsedRequest{
        .method = request.m_method,
        .version = request.m_version,
        .target = request.m_target,
        .headers = request.m_headers.Stringify(),
        .body = request.m_body,
        .consumed = reader.Consumed(),
    };
}

void AssertSameRequest(const ParsedRequest& actual, const ParsedRequest& expected)
{
    assert(actual.method == expected.method);
    assert(actual.version.major == expected.version.major);
    assert(actual.version.minor == expected.version.minor);
    assert(actual.target == expected.target);
    assert(actual.headers == expected.headers);
    assert(actual.body == expected.body);
    assert(actual.consumed == expected.consumed);
}

void AssertIncrementalParsing(std::span<const std::byte> complete_request,
                              const ParsedRequest& expected)
{
    // HTTPRemoteClient creates a fresh HTTPRequest whenever the receive buffer grows.
    // Replaying prefixes therefore models its incomplete-buffer contract without
    // incorrectly reusing partially populated request state.
    const size_t stride{std::max<size_t>(1, complete_request.size() / 16)};
    for (size_t cut{stride}; cut < complete_request.size(); cut += stride) {
        const auto parsed{ParseRequest(complete_request.first(cut))};
        if (parsed) AssertSameRequest(*parsed, expected);
    }
}

} // namespace


std::string_view RequestMethodString(HTTPRequestMethod m);

FUZZ_TARGET(http_request)
{
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<std::byte> http_buffer{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 4096)};

    HTTPRequest http_request;
    LineReader reader(http_buffer, MAX_HEADERS_SIZE);
    try {
        if (!http_request.LoadControlData(reader)) return;
        if (!http_request.LoadHeaders(reader)) return;
        const size_t body_start{reader.Consumed()};
        if (!http_request.LoadBody(reader)) return;
        AssertBodyContract(http_request, reader, body_start, http_buffer.size());
    } catch (const std::runtime_error&) {
        return;
    }

    const ParsedRequest parsed{
        .method = http_request.m_method,
        .version = http_request.m_version,
        .target = http_request.m_target,
        .headers = http_request.m_headers.Stringify(),
        .body = http_request.m_body,
        .consumed = reader.Consumed(),
    };
    AssertIncrementalParsing(std::span<const std::byte>{http_buffer}.first(parsed.consumed), parsed);

    const HTTPRequestMethod request_method = http_request.GetRequestMethod();
    (void)RequestMethodString(request_method);
    (void)http_request.GetURI();
    (void)http_request.GetHeader("Host");
    std::string header = fuzzed_data_provider.ConsumeRandomLengthString(16);
    (void)http_request.GetHeader(header);
    (void)http_request.WriteHeader(std::string(header), fuzzed_data_provider.ConsumeRandomLengthString(16));
    (void)http_request.GetHeader(header);
    const std::string body = http_request.ReadBody();
    assert(body == parsed.body);
}
