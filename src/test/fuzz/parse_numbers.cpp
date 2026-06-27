// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstdint>
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
            assert(LocaleIndependentAtoi<int8_t>(random_string) == *i8);
        }
        if (u8) {
            assert(u8 == u64);
            assert(LocaleIndependentAtoi<uint8_t>(random_string) == *u8);
        }
        if (i16) {
            assert(i16 == i64);
            assert(LocaleIndependentAtoi<int16_t>(random_string) == *i16);
        }
        if (u16) {
            assert(u16 == u64);
            assert(LocaleIndependentAtoi<uint16_t>(random_string) == *u16);
        }
        if (i32) {
            assert(i32 == i64);
            assert(LocaleIndependentAtoi<int32_t>(random_string) == *i32);
        }
        if (u32) {
            assert(u32 == u64);
            assert(LocaleIndependentAtoi<uint32_t>(random_string) == *u32);
        }
        constexpr auto digits{"0123456789"};
        if (i64) {
            assert(util::RemovePrefixView(random_string, "-").find_first_not_of(digits) == std::string::npos);
            assert(LocaleIndependentAtoi<int64_t>(random_string) == *i64);
        }
        if (u64) {
            assert(random_string.find_first_not_of(digits) == std::string::npos);
            assert(LocaleIndependentAtoi<uint64_t>(random_string) == *u64);
        }
    }

    if (const std::optional<CAmount> parsed_money{ParseMoney(random_string)}) {
        assert(MoneyRange(*parsed_money));

        const std::string formatted_money{FormatMoney(*parsed_money)};
        const std::optional<CAmount> reparsed_money{ParseMoney(formatted_money)};
        assert(reparsed_money);
        assert(*reparsed_money == *parsed_money);

        int64_t parsed_fixed_point;
        assert(ParseFixedPoint(formatted_money, 8, &parsed_fixed_point));
        assert(parsed_fixed_point == *parsed_money);
    }

    (void)LocaleIndependentAtoi<int>(random_string);

    (void)LocaleIndependentAtoi<int64_t>(random_string);
    int64_t parsed_fixed_point_3{0};
    const bool parsed_3{ParseFixedPoint(random_string, 3, &parsed_fixed_point_3)};
    int64_t parsed_fixed_point_4{0};
    const bool parsed_4{ParseFixedPoint(random_string, 4, &parsed_fixed_point_4)};
    if (parsed_3 && parsed_4) {
        assert(parsed_fixed_point_4 % 10 == 0);
        assert(parsed_fixed_point_4 / 10 == parsed_fixed_point_3);
    }
}
