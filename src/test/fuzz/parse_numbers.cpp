// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/moneystr.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {
constexpr std::array BYTE_UNITS{
    std::pair{'\0', ByteUnit::NOOP},
    std::pair{'k', ByteUnit::k},
    std::pair{'K', ByteUnit::K},
    std::pair{'m', ByteUnit::m},
    std::pair{'M', ByteUnit::M},
    std::pair{'g', ByteUnit::g},
    std::pair{'G', ByteUnit::G},
    std::pair{'t', ByteUnit::t},
    std::pair{'T', ByteUnit::T},
};

std::optional<ByteUnit> ByteUnitFromSuffix(char suffix)
{
    switch (suffix) {
    case 'k': return ByteUnit::k;
    case 'K': return ByteUnit::K;
    case 'm': return ByteUnit::m;
    case 'M': return ByteUnit::M;
    case 'g': return ByteUnit::g;
    case 'G': return ByteUnit::G;
    case 't': return ByteUnit::t;
    case 'T': return ByteUnit::T;
    }
    return std::nullopt;
}

std::optional<uint64_t> ParseDecimalUInt64(std::string_view digits)
{
    if (digits.empty()) return std::nullopt;

    uint64_t value{0};
    for (const char digit : digits) {
        if (digit < '0' || digit > '9') return std::nullopt;
        const uint64_t increment{static_cast<uint64_t>(digit - '0')};
        if (value > (std::numeric_limits<uint64_t>::max() - increment) / 10) {
            return std::nullopt;
        }
        value = value * 10 + increment;
    }
    return value;
}

std::optional<uint64_t> ExpectedParseByteUnits(std::string_view input, ByteUnit default_multiplier)
{
    if (input.empty()) return std::nullopt;

    ByteUnit multiplier{default_multiplier};
    if (const std::optional<ByteUnit> suffix_multiplier{ByteUnitFromSuffix(input.back())}) {
        multiplier = *suffix_multiplier;
        input.remove_suffix(1);
    }

    const std::optional<uint64_t> parsed_num{ParseDecimalUInt64(input)};
    if (!parsed_num) return std::nullopt;

    const uint64_t unit_amount{static_cast<uint64_t>(multiplier)};
    if (*parsed_num > std::numeric_limits<uint64_t>::max() / unit_amount) {
        return std::nullopt;
    }
    return *parsed_num * unit_amount;
}

void AssertParseByteUnitsContracts(std::string_view input, ByteUnit default_multiplier)
{
    const std::optional<uint64_t> parsed{ParseByteUnits(input, default_multiplier)};
    assert(parsed == ExpectedParseByteUnits(input, default_multiplier));
}
} // namespace

FUZZ_TARGET(parse_numbers)
{
    const std::string random_string(buffer.begin(), buffer.end());
    FuzzedDataProvider provider(buffer.data(), buffer.size());
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

    const int atoi_int{LocaleIndependentAtoi<int>(random_string)};
    const int64_t atoi_int64{LocaleIndependentAtoi<int64_t>(random_string)};
    assert(atoi_int == std::clamp(
        atoi_int64,
        static_cast<int64_t>(std::numeric_limits<int>::min()),
        static_cast<int64_t>(std::numeric_limits<int>::max())));

    int64_t parsed_fixed_point_3{0};
    const bool parsed_3{ParseFixedPoint(random_string, 3, &parsed_fixed_point_3)};
    int64_t parsed_fixed_point_4{0};
    const bool parsed_4{ParseFixedPoint(random_string, 4, &parsed_fixed_point_4)};
    if (parsed_3 && parsed_4) {
        assert(parsed_fixed_point_4 % 10 == 0);
        assert(parsed_fixed_point_4 / 10 == parsed_fixed_point_3);
    }

    AssertParseByteUnitsContracts(random_string, provider.PickValueInArray(BYTE_UNITS).second);

    const uint64_t byte_unit_amount{provider.ConsumeIntegral<uint64_t>()};
    const auto [suffix, suffix_multiplier]{provider.PickValueInArray(BYTE_UNITS)};
    const auto default_multiplier{provider.PickValueInArray(BYTE_UNITS).second};
    std::string byte_units_input{std::to_string(byte_unit_amount)};
    if (suffix != '\0') byte_units_input.push_back(suffix);
    const uint64_t multiplier{static_cast<uint64_t>(suffix == '\0' ? default_multiplier : suffix_multiplier)};
    const std::optional<uint64_t> parsed_bytes{ParseByteUnits(byte_units_input, default_multiplier)};
    if (byte_unit_amount > std::numeric_limits<uint64_t>::max() / multiplier) {
        assert(!parsed_bytes);
    } else {
        assert(parsed_bytes);
        assert(*parsed_bytes == byte_unit_amount * multiplier);
    }
}
