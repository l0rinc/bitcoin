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

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>


std::string_view RequestMethodString(HTTPRequestMethod m);

FUZZ_TARGET(http_request)
{
    using http_bitcoin::HTTPRequest;
    using http_bitcoin::MAX_BODY_SIZE;
    using http_bitcoin::MAX_HEADERS_SIZE;
    using util::LineReader;

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
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
