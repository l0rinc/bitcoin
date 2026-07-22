// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

namespace {
bool IsValidAddition(const int64_t lhs, const int64_t rhs)
{
    return rhs == 0 || (rhs > 0 && lhs <= std::numeric_limits<int64_t>::max() - rhs) || (rhs < 0 && lhs >= std::numeric_limits<int64_t>::min() - rhs);
}

bool IsValidSubtraction(const int64_t lhs, const int64_t rhs)
{
    return rhs == 0 || (rhs > 0 && lhs >= std::numeric_limits<int64_t>::min() + rhs) || (rhs < 0 && lhs <= std::numeric_limits<int64_t>::max() + rhs);
}

std::vector<unsigned char> ReferenceSerialize(const int64_t value)
{
    if (value == 0) return {};

    const bool negative{value < 0};
    uint64_t absolute{negative ? ~static_cast<uint64_t>(value) + 1 : static_cast<uint64_t>(value)};
    std::vector<unsigned char> result;
    while (absolute != 0) {
        result.push_back(static_cast<unsigned char>(absolute));
        absolute >>= 8;
    }

    if (result.back() & 0x80) {
        result.push_back(negative ? 0x80 : 0);
    } else if (negative) {
        result.back() |= 0x80;
    }
    return result;
}

int ReferenceGetInt(const int64_t value)
{
    if (value > std::numeric_limits<int>::max()) return std::numeric_limits<int>::max();
    if (value < std::numeric_limits<int>::min()) return std::numeric_limits<int>::min();
    return static_cast<int>(value);
}

void AssertScriptNumContracts(const CScriptNum& actual, const int64_t expected)
{
    assert(actual.GetInt64() == expected);
    assert(actual.getint() == ReferenceGetInt(expected));

    const std::vector<unsigned char> expected_bytes{ReferenceSerialize(expected)};
    assert(actual.getvch() == expected_bytes);
    if (expected_bytes.size() <= CScriptNum::nDefaultMaxNumSize) {
        const CScriptNum round_tripped{expected_bytes, /* fRequireMinimal= */ true};
        assert(round_tripped.GetInt64() == expected);
        assert(round_tripped.getvch() == expected_bytes);
    }
}
} // namespace

FUZZ_TARGET(scriptnum_ops)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScriptNum script_num = ConsumeScriptNum(fuzzed_data_provider);
    int64_t expected{script_num.GetInt64()};
    AssertScriptNumContracts(script_num, expected);
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 1000000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const int64_t i = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                assert((script_num == i) == (expected == i));
                assert((script_num != i) == (expected != i));
                assert((script_num < i) == (expected < i));
                assert((script_num <= i) == (expected <= i));
                assert((script_num > i) == (expected > i));
                assert((script_num >= i) == (expected >= i));
                // Avoid signed integer overflow:
                if (IsValidAddition(expected, i)) {
                    const int64_t expected_sum{expected + i};
                    assert((script_num + i).GetInt64() == expected_sum);
                }
                if (IsValidSubtraction(expected, i)) {
                    const int64_t expected_difference{expected - i};
                    assert((script_num - i).GetInt64() == expected_difference);
                }
            },
            [&] {
                const CScriptNum random_script_num = ConsumeScriptNum(fuzzed_data_provider);
                const int64_t rhs{random_script_num.GetInt64()};
                assert((script_num == random_script_num) == (expected == rhs));
                assert((script_num != random_script_num) == (expected != rhs));
                assert((script_num < random_script_num) == (expected < rhs));
                assert((script_num <= random_script_num) == (expected <= rhs));
                assert((script_num > random_script_num) == (expected > rhs));
                assert((script_num >= random_script_num) == (expected >= rhs));
                if (IsValidAddition(expected, rhs)) {
                    assert((script_num + random_script_num).GetInt64() == expected + rhs);
                }
                if (IsValidSubtraction(expected, rhs)) {
                    assert((script_num - random_script_num).GetInt64() == expected - rhs);
                }
            },
            [&] {
                const CScriptNum random_script_num = ConsumeScriptNum(fuzzed_data_provider);
                const int64_t rhs{random_script_num.GetInt64()};
                if (!IsValidAddition(expected, rhs)) {
                    return;
                }
                expected += rhs;
                script_num += random_script_num;
            },
            [&] {
                const CScriptNum random_script_num = ConsumeScriptNum(fuzzed_data_provider);
                const int64_t rhs{random_script_num.GetInt64()};
                if (!IsValidSubtraction(expected, rhs)) {
                    return;
                }
                expected -= rhs;
                script_num -= random_script_num;
            },
            [&] {
                const int64_t rhs{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
                expected &= rhs;
                script_num = script_num & rhs;
            },
            [&] {
                const CScriptNum rhs{ConsumeScriptNum(fuzzed_data_provider)};
                expected &= rhs.GetInt64();
                script_num = script_num & rhs;
            },
            [&] {
                const CScriptNum rhs{ConsumeScriptNum(fuzzed_data_provider)};
                expected &= rhs.GetInt64();
                script_num &= rhs;
            },
            [&] {
                if (expected == std::numeric_limits<int64_t>::min()) {
                    return;
                }
                expected = -expected;
                script_num = -script_num;
            },
            [&] {
                expected = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                script_num = expected;
            },
            [&] {
                const int64_t random_integer = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                if (!IsValidAddition(expected, random_integer)) {
                    return;
                }
                expected += random_integer;
                script_num += random_integer;
            },
            [&] {
                const int64_t random_integer = fuzzed_data_provider.ConsumeIntegral<int64_t>();
                if (!IsValidSubtraction(expected, random_integer)) {
                    return;
                }
                expected -= random_integer;
                script_num -= random_integer;
            },
            [&] {
                const int64_t rhs{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
                expected &= rhs;
                script_num &= rhs;
            });
        AssertScriptNumContracts(script_num, expected);
    }
}
