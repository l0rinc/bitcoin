// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view BECH32_CHARSET{"qpzry9x8gf2tvdw0s3jn54khce6mua7l"};

char MutatedBech32Char(char c)
{
    for (const char candidate : BECH32_CHARSET) {
        if (candidate != c) return candidate;
    }
    assert(false);
    return BECH32_CHARSET.front();
}

void CheckSingleCharacterMutation(const std::string& encoded, size_t mutate_pos)
{
    assert(mutate_pos < encoded.size());

    std::string mutated{encoded};
    mutated[mutate_pos] = MutatedBech32Char(mutated[mutate_pos]);

    const auto decoded = bech32::Decode(mutated);
    assert(decoded.encoding == bech32::Encoding::INVALID);
    assert(decoded.hrp.empty());
    assert(decoded.data.empty());

    const auto [error, error_locations] = bech32::LocateErrors(mutated);
    assert(!error.empty());

    const size_t separator_pos{encoded.rfind(bech32::SEPARATOR)};
    assert(separator_pos != encoded.npos);
    if (mutate_pos > separator_pos) {
        assert((error_locations == std::vector<int>{static_cast<int>(mutate_pos)}));
    }
}

} // namespace

FUZZ_TARGET(bech32_random_decode)
{
    auto limit = bech32::CharLimit::BECH32;
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    auto random_string = fdp.ConsumeRandomLengthString(limit + 1);
    auto decoded = bech32::Decode(random_string, limit);
    const auto [error, error_locations] = bech32::LocateErrors(random_string, limit);

    if (decoded.hrp.empty()) {
        assert(decoded.encoding == bech32::Encoding::INVALID);
        assert(decoded.data.empty());
        assert(!error.empty());
    } else {
        assert(decoded.encoding != bech32::Encoding::INVALID);
        assert(error.empty());
        assert(error_locations.empty());
        auto reencoded = bech32::Encode(decoded.encoding, decoded.hrp, decoded.data);
        assert(CaseInsensitiveEqual(random_string, reencoded));
        const auto [reencoded_error, reencoded_error_locations] = bech32::LocateErrors(reencoded, limit);
        assert(reencoded_error.empty());
        assert(reencoded_error_locations.empty());
    }
}

// https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki and https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki
std::string GenerateRandomHRP(FuzzedDataProvider& fdp)
{
    std::string hrp;
    size_t length = fdp.ConsumeIntegralInRange<size_t>(1, 83);
    for (size_t i = 0; i < length; ++i) {
        // Generate lowercase ASCII characters in ([33-126] - ['A'-'Z']) range
        char c = fdp.ConsumeBool()
                 ? fdp.ConsumeIntegralInRange<char>(33, 'A' - 1)
                 : fdp.ConsumeIntegralInRange<char>('Z' + 1, 126);
        hrp += c;
    }
    return hrp;
}

FUZZ_TARGET(bech32_roundtrip)
{
    FuzzedDataProvider fdp(buffer.data(), buffer.size());
    auto hrp = GenerateRandomHRP(fdp);

    auto input_chars = fdp.ConsumeBytes<unsigned char>(fdp.ConsumeIntegralInRange<size_t>(0, 82));
    std::vector<uint8_t> converted_input;
    ConvertBits<8, 5, true>([&](auto c) { converted_input.push_back(c); }, input_chars.begin(), input_chars.end());

    auto size = converted_input.size() + hrp.length() + std::string({bech32::SEPARATOR}).size() + bech32::CHECKSUM_SIZE;
    if (size <= bech32::CharLimit::BECH32) {
        for (auto encoding: {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
            auto encoded = bech32::Encode(encoding, hrp, converted_input);
            assert(!encoded.empty());

            const auto decoded = bech32::Decode(encoded);
            assert(decoded.encoding == encoding);
            assert(decoded.hrp == hrp);
            assert(decoded.data == converted_input);

            const auto [error, error_locations] = bech32::LocateErrors(encoded);
            assert(error.empty());
            assert(error_locations.empty());

            const size_t mutate_pos = fdp.ConsumeIntegralInRange<size_t>(0, encoded.size() - 1);
            CheckSingleCharacterMutation(encoded, mutate_pos);
        }
    }
}
