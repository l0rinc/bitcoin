// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/overflow.h>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
template <typename T>
void TestMultiplicationPair(const T i, const T j)
{
    const bool is_multiplication_overflow_custom = MultiplicationOverflow(i, j);
    const auto checked_mul{CheckedMul(i, j)};
    const auto saturating_mul{SaturatingMul(i, j)};
    assert(is_multiplication_overflow_custom == MultiplicationOverflow(j, i));
    assert(checked_mul == CheckedMul(j, i));
    assert(saturating_mul == SaturatingMul(j, i));
    assert(is_multiplication_overflow_custom == !checked_mul.has_value());
#ifndef _MSC_VER
    T result_builtin;
    const bool is_multiplication_overflow_builtin = __builtin_mul_overflow(i, j, &result_builtin);
    assert(is_multiplication_overflow_custom == is_multiplication_overflow_builtin);
    if (!is_multiplication_overflow_custom) {
        assert(*checked_mul == result_builtin);
        assert(saturating_mul == result_builtin);
        assert(i * j == result_builtin);
    }
#else
    if (!is_multiplication_overflow_custom) {
        (void)(i * j);
        assert(saturating_mul == *checked_mul);
    }
#endif
    if (is_multiplication_overflow_custom) {
        if constexpr (std::numeric_limits<T>::is_signed) {
            const T saturation_value{(i < 0) != (j < 0) ?
                                         std::numeric_limits<T>::min() :
                                         std::numeric_limits<T>::max()};
            assert(saturating_mul == saturation_value);
        } else {
            assert(saturating_mul == std::numeric_limits<T>::max());
        }
    }
}

template <typename T>
constexpr auto BoundaryValues()
{
    if constexpr (std::numeric_limits<T>::is_signed) {
        return std::array{
            std::numeric_limits<T>::min(),
            static_cast<T>(std::numeric_limits<T>::min() + T{1}),
            T{-2},
            T{-1},
            T{0},
            T{1},
            T{2},
            static_cast<T>(std::numeric_limits<T>::max() - T{1}),
            std::numeric_limits<T>::max(),
        };
    } else {
        return std::array{
            std::numeric_limits<T>::min(),
            T{1},
            T{2},
            static_cast<T>(std::numeric_limits<T>::max() / T{2}),
            static_cast<T>(std::numeric_limits<T>::max() - T{1}),
            std::numeric_limits<T>::max(),
        };
    }
}

template <typename T>
void TestMultiplicationOverflow(FuzzedDataProvider& fuzzed_data_provider)
{
    TestMultiplicationPair<T>(fuzzed_data_provider.ConsumeIntegral<T>(), fuzzed_data_provider.ConsumeIntegral<T>());
    for (const T i : BoundaryValues<T>()) {
        for (const T j : BoundaryValues<T>()) {
            TestMultiplicationPair<T>(i, j);
        }
    }
}
} // namespace

FUZZ_TARGET(multiplication_overflow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    TestMultiplicationOverflow<int64_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint64_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<int32_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint32_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<int16_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<uint16_t>(fuzzed_data_provider);
    TestMultiplicationOverflow<char>(fuzzed_data_provider);
    TestMultiplicationOverflow<unsigned char>(fuzzed_data_provider);
    TestMultiplicationOverflow<signed char>(fuzzed_data_provider);
}
