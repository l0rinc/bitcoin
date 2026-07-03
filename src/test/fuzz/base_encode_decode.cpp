// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>

#include <base58.h>
#include <psbt.h>
#include <span.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <string>
#include <string_view>
#include <vector>
#include <ranges>

using util::TrimStringView;

namespace {
std::vector<unsigned char> SerializePSBT(const PartiallySignedTransaction& psbt)
{
    std::vector<unsigned char> psbt_ser;
    VectorWriter{psbt_ser, 0, psbt};
    return psbt_ser;
}

std::string MutateTrailingPaddingBits(std::string encoded, std::string_view alphabet)
{
    const auto padding_pos{encoded.find('=')};
    assert(padding_pos != std::string::npos);
    assert(padding_pos > 0);
    const auto alphabet_pos{alphabet.find(encoded[padding_pos - 1])};
    assert(alphabet_pos != std::string_view::npos);
    assert((alphabet_pos & 1U) == 0);
    encoded[padding_pos - 1] = alphabet[alphabet_pos | 1U];
    return encoded;
}
} // namespace

FUZZ_TARGET(base58_encode_decode)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const auto random_string{provider.ConsumeRandomLengthString(100)};

    const auto encoded{EncodeBase58(MakeUCharSpan(random_string))};
    std::vector<unsigned char> decoded_encoded;
    assert(DecodeBase58(encoded, decoded_encoded, static_cast<int>(random_string.size())));
    assert(std::ranges::equal(decoded_encoded, MakeUCharSpan(random_string)));
    assert(EncodeBase58(decoded_encoded) == encoded);
    const auto decode_input{provider.ConsumeBool() ? random_string : encoded};
    const int max_ret_len{provider.ConsumeIntegralInRange<int>(-1, decode_input.size() + 1)};
    std::vector<unsigned char> decoded{0x42};
    if (DecodeBase58(decode_input, decoded, max_ret_len)) {
        const auto encoded_string{EncodeBase58(decoded)};
        assert(encoded_string == TrimStringView(decode_input));
        if (decoded.size() > 0) {
            assert(max_ret_len > 0);
            assert(decoded.size() <= static_cast<size_t>(max_ret_len));
            assert(!DecodeBase58(encoded_string, decoded, provider.ConsumeIntegralInRange<int>(0, decoded.size() - 1)));
            assert(decoded.empty());
        }
    } else {
        assert(decoded.empty());
    }
}

FUZZ_TARGET(base58check_encode_decode)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const auto random_string{provider.ConsumeRandomLengthString(100)};

    const auto encoded{EncodeBase58Check(MakeUCharSpan(random_string))};
    std::vector<unsigned char> decoded_encoded;
    assert(DecodeBase58Check(encoded, decoded_encoded, static_cast<int>(random_string.size())));
    assert(std::ranges::equal(decoded_encoded, MakeUCharSpan(random_string)));
    assert(EncodeBase58Check(decoded_encoded) == encoded);
    const auto decode_input{provider.ConsumeBool() ? random_string : encoded};
    const int max_ret_len{provider.ConsumeIntegralInRange<int>(-1, decode_input.size() + 1)};
    std::vector<unsigned char> decoded{0x42};
    if (DecodeBase58Check(decode_input, decoded, max_ret_len)) {
        const auto encoded_string{EncodeBase58Check(decoded)};
        assert(encoded_string == TrimStringView(decode_input));
        if (decoded.size() > 0) {
            assert(max_ret_len > 0);
            assert(decoded.size() <= static_cast<size_t>(max_ret_len));
            assert(!DecodeBase58Check(encoded_string, decoded, provider.ConsumeIntegralInRange<int>(0, decoded.size() - 1)));
        }
    } else {
        assert(decoded.empty());
    }
}

FUZZ_TARGET(base32_encode_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase32(random_string)}) {
        const auto encoded_string{EncodeBase32(*result)};
        assert(encoded_string == ToLower(TrimStringView(random_string)));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase32(buffer)};
    if (!encoded.empty()) {
        std::string leading_padding{encoded};
        leading_padding.front() = '=';
        assert(!DecodeBase32(leading_padding));
    }
    const auto decoded{DecodeBase32(encoded)};
    assert(decoded && std::ranges::equal(*decoded, buffer));
    if (encoded.find('=') != std::string::npos) {
        assert(!DecodeBase32(MutateTrailingPaddingBits(encoded, "abcdefghijklmnopqrstuvwxyz234567")));
    }

    const auto unpadded{EncodeBase32(buffer, false)};
    assert(encoded.starts_with(unpadded));
    assert(encoded.find_first_not_of('=', unpadded.size()) == std::string::npos);
    const auto decoded_unpadded{DecodeBase32(unpadded)};
    if (unpadded.size() % 8 == 0) {
        assert(decoded_unpadded && std::ranges::equal(*decoded_unpadded, buffer));
    } else {
        assert(!decoded_unpadded);
    }
}

FUZZ_TARGET(base64_encode_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    // Decode/Encode roundtrip
    if (auto result{DecodeBase64(random_string)}) {
        const auto encoded_string{EncodeBase64(*result)};
        assert(encoded_string == TrimStringView(random_string));
    }
    // Encode/Decode roundtrip
    const auto encoded{EncodeBase64(buffer)};
    if (!encoded.empty()) {
        std::string leading_padding{encoded};
        leading_padding.front() = '=';
        assert(!DecodeBase64(leading_padding));
    }
    const auto decoded{DecodeBase64(encoded)};
    assert(decoded && std::ranges::equal(*decoded, buffer));
    if (encoded.find('=') != std::string::npos) {
        assert(!DecodeBase64(MutateTrailingPaddingBits(encoded, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/")));
    }
}

FUZZ_TARGET(psbt_base64_decode)
{
    const std::string random_string{buffer.begin(), buffer.end()};

    util::Result<PartiallySignedTransaction> psbt = DecodeBase64PSBT(random_string);
    if (!psbt) {
        return;
    }

    const auto decoded{DecodeBase64(random_string)};
    assert(decoded);
    assert(EncodeBase64(*decoded) == TrimStringView(random_string));

    const auto serialized{SerializePSBT(*psbt)};
    const auto encoded{EncodeBase64(serialized)};
    const util::Result<PartiallySignedTransaction> roundtrip{DecodeBase64PSBT(encoded)};
    assert(roundtrip);
    assert(SerializePSBT(*roundtrip) == serialized);
}
