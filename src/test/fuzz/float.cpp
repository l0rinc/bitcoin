// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memusage.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/serfloat.h>

#include <cassert>
#include <cmath>
#include <limits>
#include <optional>

namespace {

constexpr uint64_t DOUBLE_EXP_MASK{0x7FF0000000000000};
constexpr uint64_t DOUBLE_MAN_MASK{0x000FFFFFFFFFFFFF};
constexpr uint64_t DOUBLE_CANONICAL_NAN{0x7ff8000000000000};

bool IsDoubleNaNEncoding(uint64_t encoded)
{
    return (encoded & DOUBLE_EXP_MASK) == DOUBLE_EXP_MASK && (encoded & DOUBLE_MAN_MASK) != 0;
}

void AssertDoubleEncodingContract(uint64_t raw)
{
    const double decoded{DecodeDouble(raw)};
    const uint64_t reencoded{EncodeDouble(decoded)};

    if (IsDoubleNaNEncoding(raw)) {
        assert(std::isnan(decoded));
        assert(reencoded == DOUBLE_CANONICAL_NAN);
    } else {
        assert(!std::isnan(decoded));
        assert(reencoded == raw);
    }

    const double decoded_again{DecodeDouble(reencoded)};
    assert(std::isnan(decoded) == std::isnan(decoded_again));
    assert(std::isnan(decoded) || decoded == decoded_again);
}

} // namespace

FUZZ_TARGET(float)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    AssertDoubleEncodingContract(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    AssertDoubleEncodingContract(0x0000000000000000);
    AssertDoubleEncodingContract(0x8000000000000000);
    AssertDoubleEncodingContract(0x0000000000000001);
    AssertDoubleEncodingContract(0x8000000000000001);
    AssertDoubleEncodingContract(0x7ff0000000000000);
    AssertDoubleEncodingContract(0xfff0000000000000);
    AssertDoubleEncodingContract(0x7ff0000000000001);
    AssertDoubleEncodingContract(0xfff0000000000001);
    AssertDoubleEncodingContract(0x7fffffffffffffff);

    {
        const double d{[&] {
            std::optional<double> tmp;
            CallOneOf(
                fuzzed_data_provider,
                // an actual number
                [&] { tmp = fuzzed_data_provider.ConsumeFloatingPoint<double>(); },
                // special numbers and NANs
                [&] { tmp = fuzzed_data_provider.PickValueInArray({
                          std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::min(),
                          -std::numeric_limits<double>::min(),
                          std::numeric_limits<double>::max(),
                          -std::numeric_limits<double>::max(),
                          std::numeric_limits<double>::lowest(),
                          -std::numeric_limits<double>::lowest(),
                          std::numeric_limits<double>::quiet_NaN(),
                          -std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::signaling_NaN(),
                          -std::numeric_limits<double>::signaling_NaN(),
                          std::numeric_limits<double>::denorm_min(),
                          -std::numeric_limits<double>::denorm_min(),
                      }); },
                // Anything from raw memory (also checks that DecodeDouble doesn't crash on any input)
                [&] { tmp = DecodeDouble(fuzzed_data_provider.ConsumeIntegral<uint64_t>()); });
            return *tmp;
        }()};
        (void)memusage::DynamicUsage(d);

        uint64_t encoded = EncodeDouble(d);
        if (std::isnan(d)) {
            assert(encoded == DOUBLE_CANONICAL_NAN);
        }
        if constexpr (std::numeric_limits<double>::is_iec559) {
            if (!std::isnan(d)) {
                uint64_t encoded_in_memory;
                std::copy((const unsigned char*)&d, (const unsigned char*)(&d + 1), (unsigned char*)&encoded_in_memory);
                assert(encoded_in_memory == encoded);
            }
        }
        double d_deserialized = DecodeDouble(encoded);
        assert(std::isnan(d) == std::isnan(d_deserialized));
        assert(std::isnan(d) || d == d_deserialized);
        assert(EncodeDouble(d_deserialized) == encoded);
    }
}
