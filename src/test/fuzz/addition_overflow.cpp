// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/overflow.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
template <typename T>
void TestAdditionPair(const T i, const T j)
{
    const bool is_addition_overflow_custom = AdditionOverflow(i, j);
    const auto maybe_add{CheckedAdd(i, j)};
    const auto sat_add{SaturatingAdd(i, j)};
    assert(is_addition_overflow_custom == !maybe_add.has_value());
    assert(is_addition_overflow_custom == AdditionOverflow(j, i));
    assert(maybe_add == CheckedAdd(j, i));
    assert(sat_add == SaturatingAdd(j, i));
#ifndef _MSC_VER
    T result_builtin;
    const bool is_addition_overflow_builtin = __builtin_add_overflow(i, j, &result_builtin);
    assert(is_addition_overflow_custom == is_addition_overflow_builtin);
    if (!is_addition_overflow_custom) {
        assert(i + j == result_builtin);
    }
#endif
    if (is_addition_overflow_custom) {
        assert(sat_add == std::numeric_limits<T>::min() || sat_add == std::numeric_limits<T>::max());
    } else {
        const auto add{i + j};
        assert(add == maybe_add.value());
        assert(add == sat_add);
    }
}

template <typename T>
T ConsumeBoundaryValue(FuzzedDataProvider& fuzzed_data_provider)
{
    if constexpr (std::numeric_limits<T>::is_signed) {
        return fuzzed_data_provider.PickValueInArray(std::array{
            std::numeric_limits<T>::min(),
            static_cast<T>(std::numeric_limits<T>::min() + T{1}),
            T{-1},
            T{0},
            T{1},
            static_cast<T>(std::numeric_limits<T>::max() - T{1}),
            std::numeric_limits<T>::max(),
        });
    } else {
        return fuzzed_data_provider.PickValueInArray(std::array{
            std::numeric_limits<T>::min(),
            T{1},
            static_cast<T>(std::numeric_limits<T>::max() - T{1}),
            std::numeric_limits<T>::max(),
        });
    }
}

template <typename T>
void TestAdditionOverflow(FuzzedDataProvider& fuzzed_data_provider)
{
    TestAdditionPair<T>(fuzzed_data_provider.ConsumeIntegral<T>(), fuzzed_data_provider.ConsumeIntegral<T>());
    TestAdditionPair<T>(ConsumeBoundaryValue<T>(fuzzed_data_provider), ConsumeBoundaryValue<T>(fuzzed_data_provider));
}
} // namespace

FUZZ_TARGET(addition_overflow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TestAdditionOverflow<int64_t>(fuzzed_data_provider);
    TestAdditionOverflow<uint64_t>(fuzzed_data_provider);
    TestAdditionOverflow<int32_t>(fuzzed_data_provider);
    TestAdditionOverflow<uint32_t>(fuzzed_data_provider);
    TestAdditionOverflow<int16_t>(fuzzed_data_provider);
    TestAdditionOverflow<uint16_t>(fuzzed_data_provider);
    TestAdditionOverflow<char>(fuzzed_data_provider);
    TestAdditionOverflow<unsigned char>(fuzzed_data_provider);
    TestAdditionOverflow<signed char>(fuzzed_data_provider);
}
