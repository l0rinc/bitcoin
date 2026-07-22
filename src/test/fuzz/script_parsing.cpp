// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/parsing.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/string.h>

#include <algorithm>
#include <string_view>
#include <vector>

using util::Split;

namespace {

void AssertSpanEqual(const std::span<const char> actual, const std::span<const char> expected)
{
    Assert(actual.size() == expected.size());
    Assert(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()));
}

void AssertSpanText(const std::span<const char> actual, const std::string_view expected)
{
    Assert(actual.size() == expected.size());
    Assert(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()));
}

void AssertConstContract(const std::string& query, const std::span<const char> original)
{
    const std::string original_string{original.begin(), original.end()};
    const bool expected_match{original.size() >= query.size() && std::equal(query.begin(), query.end(), original.begin())};

    std::span<const char> skipped{original};
    const bool skip_result{script::Const(query, skipped)};
    Assert(skip_result == expected_match);
    if (expected_match) {
        AssertSpanText(skipped, std::string_view{original_string}.substr(query.size()));
    } else {
        AssertSpanEqual(skipped, original);
    }

    std::span<const char> peeked{original};
    const bool peek_result{script::Const(query, peeked, /*skip=*/false)};
    Assert(peek_result == expected_match);
    AssertSpanEqual(peeked, original);
}

void AssertFuncContract(const std::string& query, const std::span<const char> original)
{
    const std::string original_string{original.begin(), original.end()};
    const bool expected_match{
        original.size() >= query.size() + 2 && original[query.size()] == '(' && original.back() == ')' &&
        std::equal(query.begin(), query.end(), original.begin())};

    std::span<const char> arguments{original};
    const bool result{script::Func(query, arguments)};
    Assert(result == expected_match);
    if (expected_match) {
        AssertSpanText(arguments, std::string_view{original_string}.substr(query.size() + 1, original.size() - query.size() - 2));
    } else {
        AssertSpanEqual(arguments, original);
    }
}

void AssertExprContract(const std::span<const char> original)
{
    int level{0};
    size_t expected_size{0};
    while (expected_size < original.size()) {
        const char current{original[expected_size]};
        if (current == '(' || current == '{') {
            ++level;
        } else if (level && (current == ')' || current == '}')) {
            --level;
        } else if (level == 0 && (current == ')' || current == '}' || current == ',')) {
            break;
        }
        ++expected_size;
    }

    std::span<const char> remaining{original};
    const std::span<const char> expression{script::Expr(remaining)};
    AssertSpanText(expression, std::string_view{std::string{original.begin(), original.end()}}.substr(0, expected_size));
    AssertSpanText(remaining, std::string_view{std::string{original.begin(), original.end()}}.substr(expected_size));
}

void AssertSplitContract(const std::span<const char> original, const char separator)
{
    const auto without_separator{Split(original, separator)};
    const auto with_separator{Split(original, separator, /*include_sep=*/true)};
    Assert(without_separator.size() == with_separator.size());

    std::string joined_without_separator;
    std::string joined_with_separator;
    for (size_t i{0}; i < without_separator.size(); ++i) {
        joined_without_separator.append(without_separator[i].begin(), without_separator[i].end());
        joined_with_separator.append(with_separator[i].begin(), with_separator[i].end());
        if (i + 1 < without_separator.size()) {
            joined_without_separator.push_back(separator);
            std::string expected_with_separator{without_separator[i].begin(), without_separator[i].end()};
            expected_with_separator.push_back(separator);
            const std::string actual_with_separator{with_separator[i].begin(), with_separator[i].end()};
            Assert(actual_with_separator == expected_with_separator);
        }
    }

    AssertSpanText(original, joined_without_separator);
    AssertSpanText(original, joined_with_separator);
}

void AssertParsingContracts(const std::string& query, const std::span<const char> original)
{
    AssertConstContract(query, original);
    AssertFuncContract(query, original);
    AssertExprContract(original);
    if (!query.empty()) {
        AssertSplitContract(original, query.front());
    }
}

} // namespace

FUZZ_TARGET(script_parsing)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const size_t query_size = fuzzed_data_provider.ConsumeIntegral<size_t>();
    const std::string query = fuzzed_data_provider.ConsumeBytesAsString(std::min<size_t>(query_size, 1_MiB));
    const std::string span_str = fuzzed_data_provider.ConsumeRemainingBytesAsString();
    const std::span<const char> const_span{span_str};
    AssertParsingContracts(query, const_span);
}
