// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <test/util/check.h>

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
            CHECK(i8 == i64);
        }
        if (u8) {
            CHECK(u8 == u64);
        }
        if (i16) {
            CHECK(i16 == i64);
        }
        if (u16) {
            CHECK(u16 == u64);
        }
        if (i32) {
            CHECK(i32 == i64);
        }
        if (u32) {
            CHECK(u32 == u64);
        }
        constexpr auto digits{"0123456789"};
        if (i64) {
            CHECK(util::RemovePrefixView(random_string, "-").find_first_not_of(digits) == std::string::npos);
        }
        if (u64) {
            CHECK(random_string.find_first_not_of(digits) == std::string::npos);
        }
    }

    (void)ParseMoney(random_string);

    (void)LocaleIndependentAtoi<int>(random_string);

    int64_t i64;
    (void)LocaleIndependentAtoi<int64_t>(random_string);
    (void)ParseFixedPoint(random_string, 3, &i64);
}
