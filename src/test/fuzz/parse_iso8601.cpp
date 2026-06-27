// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/time.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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

    (void)ParseISO8601DateTime(random_string);
}
