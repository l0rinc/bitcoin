// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

FUZZ_TARGET(parse_numbers)
{
    const std::string random_string(buffer.begin(), buffer.end());
    {
        const auto i8{ToIntegral<int8_t>(random_string)};
        const auto u8{ToIntegral<uint8_t>(random_string)};
        const auto i16{ToIntegral<int16_t>(random_string)};
        const auto u16{ToIntegral<uint16_t>(random_string)};
        const auto i32{ToIntegral<int32_t>(random_string)};
        const auto u32{ToIntegral<uint32_t>(random_string)};
        const auto i64{ToIntegral<int64_t>(random_string)};
        const auto u64{ToIntegral<uint64_t>(random_string)};
        // Dont check any values, just that each success result must fit into
        // the one with the largest bit-width.
        if (i8) {
            assert(i8 == i64);
        }
        if (u8) {
            assert(u8 == u64);
        }
        if (i16) {
            assert(i16 == i64);
        }
        if (u16) {
            assert(u16 == u64);
        }
        if (i32) {
            assert(i32 == i64);
        }
        if (u32) {
            assert(u32 == u64);
        }
        constexpr auto digits{"0123456789"};
        if (i64) {
            assert(util::RemovePrefixView(random_string, "-").find_first_not_of(digits) == std::string::npos);
        }
        if (u64) {
            assert(random_string.find_first_not_of(digits) == std::string::npos);
        }

        uint64_t generated{0};
        for (const auto byte : buffer) {
            generated = (generated << 8) | byte;
        }
        const std::string canonical_u64{std::to_string(generated)};
        assert(ToIntegral<uint64_t>(canonical_u64) == generated);
        assert(!ToIntegral<uint64_t>("+" + canonical_u64));
        assert(!ToIntegral<uint64_t>(" " + canonical_u64));
        assert(!ToIntegral<uint64_t>(canonical_u64 + " "));
        assert(!ToIntegral<uint64_t>(canonical_u64 + "a"));

        const uint64_t signed_mask{static_cast<uint64_t>(std::numeric_limits<int64_t>::max())};
        const int64_t signed_magnitude{static_cast<int64_t>(generated & signed_mask)};
        const int64_t generated_signed{(generated & (uint64_t{1} << 63)) != 0 ? -signed_magnitude : signed_magnitude};
        const std::string canonical_i64{std::to_string(generated_signed)};
        assert(ToIntegral<int64_t>(canonical_i64) == generated_signed);
        assert(!ToIntegral<int64_t>("+" + canonical_i64));
        assert(!ToIntegral<int64_t>(" " + canonical_i64));
        assert(!ToIntegral<int64_t>(canonical_i64 + " "));
        assert(!ToIntegral<int64_t>(canonical_i64 + "a"));
    }

    (void)ParseMoney(random_string);

    (void)LocaleIndependentAtoi<int>(random_string);

    int64_t i64;
    (void)LocaleIndependentAtoi<int64_t>(random_string);
    (void)ParseFixedPoint(random_string, 3, &i64);
}
