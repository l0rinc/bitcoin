// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/time.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {
struct ISO8601DateTimeFields {
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
};

std::optional<int> ReadDigits(std::string_view str, size_t pos, size_t len)
{
    if (pos + len > str.size()) return std::nullopt;

    int value{0};
    for (size_t i{0}; i < len; ++i) {
        const char c{str[pos + i]};
        if (c < '0' || c > '9') return std::nullopt;
        value = value * 10 + (c - '0');
    }
    return value;
}

bool IsLeapYear(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int DaysInMonth(int year, int month)
{
    switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        return IsLeapYear(year) ? 29 : 28;
    }
    return 0;
}

std::optional<ISO8601DateTimeFields> ExpectedISO8601DateTimeFields(std::string_view str)
{
    if (str.size() != std::string_view{"2000-01-01T01:01:01Z"}.size() ||
        str[4] != '-' || str[7] != '-' || str[10] != 'T' ||
        str[13] != ':' || str[16] != ':' || str[19] != 'Z') {
        return std::nullopt;
    }

    const std::optional<int> year{ReadDigits(str, 0, 4)};
    const std::optional<int> month{ReadDigits(str, 5, 2)};
    const std::optional<int> day{ReadDigits(str, 8, 2)};
    const std::optional<int> hour{ReadDigits(str, 11, 2)};
    const std::optional<int> min{ReadDigits(str, 14, 2)};
    const std::optional<int> sec{ReadDigits(str, 17, 2)};
    if (!year || !month || !day || !hour || !min || !sec) {
        return std::nullopt;
    }
    if (*month < 1 || *month > 12) return std::nullopt;
    if (*day < 1 || *day > DaysInMonth(*year, *month)) return std::nullopt;

    return ISO8601DateTimeFields{*year, *month, *day, *hour, *min, *sec};
}

void AssertISO8601ParseContracts(std::string_view str)
{
    const std::optional<int64_t> parsed{ParseISO8601DateTime(str)};
    const std::optional<ISO8601DateTimeFields> expected{ExpectedISO8601DateTimeFields(str)};
    assert(parsed.has_value() == expected.has_value());

    if (!parsed) return;

    const std::string formatted{FormatISO8601DateTime(*parsed)};
    if (formatted.size() == str.size()) {
        const std::optional<int64_t> reparsed{ParseISO8601DateTime(formatted)};
        assert(reparsed);
        assert(*reparsed == *parsed);
    }

    if (expected->hour <= 23 && expected->min <= 59 && expected->sec <= 59) {
        assert(formatted == str);
        assert(FormatISO8601Date(*parsed) == str.substr(0, 10));
    }
}
} // namespace

FUZZ_TARGET(parse_iso8601)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    constexpr int64_t SECONDS_PER_DAY{24 * 60 * 60};
    constexpr int64_t FOUR_DIGIT_YEAR_MIN{-62167219200}; // 0000-01-01T00:00:00Z
    constexpr int64_t FOUR_DIGIT_YEAR_MAX{253402300799}; // 9999-12-31T23:59:59Z

    const int64_t random_time = fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(FOUR_DIGIT_YEAR_MIN, FOUR_DIGIT_YEAR_MAX);
    const std::string random_string = fuzzed_data_provider.ConsumeRemainingBytesAsString();

    const std::string iso8601_datetime = FormatISO8601DateTime(random_time);
    const std::string iso8601_date = FormatISO8601Date(random_time);
    const int64_t parsed_time_1{ParseISO8601DateTime(iso8601_datetime).value()};
    assert(parsed_time_1 == random_time);
    assert(std::string_view{iso8601_datetime}.starts_with(iso8601_date));
    assert(iso8601_datetime.at(iso8601_date.size()) == 'T');

    const int64_t seconds_since_midnight{((random_time % SECONDS_PER_DAY) + SECONDS_PER_DAY) % SECONDS_PER_DAY};
    const int64_t day_start{random_time - seconds_since_midnight};
    const int64_t parsed_date_start{ParseISO8601DateTime(iso8601_date + "T00:00:00Z").value()};
    assert(parsed_date_start == day_start);

    AssertISO8601ParseContracts(random_string);
}
