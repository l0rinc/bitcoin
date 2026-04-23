// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <base58.h>
#include <psbt.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <string>
#include <vector>
#include <ranges>
#include <test/util/check.h>

using util::TrimStringView;

FUZZ_TARGET(base58_encode_decode)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const auto random_string{provider.ConsumeRandomLengthString(100)};

    const auto encoded{EncodeBase58(MakeUCharSpan(random_string))};
    const auto decode_input{provider.ConsumeBool() ? random_string : encoded};
    const int max_ret_len{provider.ConsumeIntegralInRange<int>(-1, decode_input.size() + 1)};
    if (std::vector<unsigned char> decoded; DecodeBase58(decode_input, decoded, max_ret_len)) {
        const auto encoded_string{EncodeBase58(decoded)};
        CHECK(encoded_string == TrimStringView(decode_input));
        if (decoded.size() > 0) {
            CHECK(max_ret_len > 0);
            CHECK(decoded.size() <= static_cast<size_t>(max_ret_len));
            CHECK(!DecodeBase58(encoded_string, decoded, provider.ConsumeIntegralInRange<int>(0, decoded.size() - 1)));
        }
    }
}

FUZZ_TARGET(base58check_encode_decode)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const auto random_string{provider.ConsumeRandomLengthString(100)};

    const auto encoded{EncodeBase58Check(MakeUCharSpan(random_string))};
    const auto decode_input{provider.ConsumeBool() ? random_string : encoded};
    const int max_ret_len{provider.ConsumeIntegralInRange<int>(-1, decode_input.size() + 1)};
    if (std::vector<unsigned char> decoded; DecodeBase58Check(decode_input, decoded, max_ret_len)) {
        const auto encoded_string{EncodeBase58Check(decoded)};
        CHECK(encoded_string == TrimStringView(decode_input));
        if (decoded.size() > 0) {
            CHECK(max_ret_len > 0);
            CHECK(decoded.size() <= static_cast<size_t>(max_ret_len));
            CHECK(!DecodeBase58Check(encoded_string, decoded, provider.ConsumeIntegralInRange<int>(0, decoded.size() - 1)));
        }
    }
}

FUZZ_TARGET(base32_encode_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase32(random_string)}) {
        const auto encoded_string{EncodeBase32(*result)};
        CHECK(encoded_string == ToLower(TrimStringView(random_string)));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase32(buffer)};
    const auto decoded{DecodeBase32(encoded)};
    CHECK(decoded && std::ranges::equal(*decoded, buffer));
}

FUZZ_TARGET(base64_encode_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase64(random_string)}) {
        const auto encoded_string{EncodeBase64(*result)};
        CHECK(encoded_string == TrimStringView(random_string));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase64(buffer)};
    const auto decoded{DecodeBase64(encoded)};
    CHECK(decoded && std::ranges::equal(*decoded, buffer));
}

FUZZ_TARGET(psbt_base64_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    PartiallySignedTransaction psbt;
    std::string error;
    const bool ok{DecodeBase64PSBT(psbt, random_string, error)};
    CHECK(ok == error.empty());
}
