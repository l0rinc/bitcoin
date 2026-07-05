// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_OVERFLOW_H
#define BITCOIN_UTIL_OVERFLOW_H

#include <util/check.h>

#include <climits>
#include <concepts>
#include <limits>
#include <optional>
#include <type_traits>

template <std::integral T>
[[nodiscard]] bool AdditionOverflow(const T i, const T j) noexcept
{
    if constexpr (std::numeric_limits<T>::is_signed) {
        return (i > 0 && j > std::numeric_limits<T>::max() - i) ||
               (i < 0 && j < std::numeric_limits<T>::min() - i);
    }
    return std::numeric_limits<T>::max() - i < j;
}

template <class T>
[[nodiscard]] std::optional<T> CheckedAdd(const T i, const T j) noexcept
{
    if (AdditionOverflow(i, j)) {
        return std::nullopt;
    }
    const T result = static_cast<T>(i + j);
    Assume(result - i == j);
    Assume(result - j == i);
    return result;
}

template <std::integral T>
[[nodiscard]] bool MultiplicationOverflow(const T i, const T j) noexcept
{
    if constexpr (std::numeric_limits<T>::is_signed) {
        if (i > 0) {
            if (j > 0) {
                return i > (std::numeric_limits<T>::max() / j);
            }
            return j < (std::numeric_limits<T>::min() / i);
        }
        if (j > 0) {
            return i < (std::numeric_limits<T>::min() / j);
        }
        return i != 0 && (j < (std::numeric_limits<T>::max() / i));
    } else {
        return j != 0 && i > std::numeric_limits<T>::max() / j;
    }
}

template <std::integral T>
[[nodiscard]] std::optional<T> CheckedMul(const T i, const T j) noexcept
{
    if (MultiplicationOverflow(i, j)) {
        return std::nullopt;
    }
    const T result = static_cast<T>(i * j);
    Assume(i == 0 || result / i == j);
    return result;
}

template <std::integral T>
[[nodiscard]] T SaturatingMul(const T i, const T j) noexcept
{
    if (const auto result{CheckedMul(i, j)}) return *result;
    if constexpr (std::numeric_limits<T>::is_signed) {
        return (i < 0) != (j < 0) ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
    } else {
        return std::numeric_limits<T>::max();
    }
}

template <std::unsigned_integral T, std::unsigned_integral U>
[[nodiscard]] constexpr bool TrySub(T& i, const U j) noexcept
{
    if (i < T{j}) return false;
    i -= T{j};
    return true;
}

template <std::integral T>
[[nodiscard]] T SaturatingAdd(const T i, const T j) noexcept
{
    if (const auto result{CheckedAdd(i, j)}) return *result;
    if constexpr (std::numeric_limits<T>::is_signed) {
        return i < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
    } else {
        return std::numeric_limits<T>::max();
    }
}

/**
 * @brief Integer ceiling division (for unsigned values).
 *
 * Computes the smallest integer q such that q * divisor >= dividend.
 * Both dividend and divisor must be unsigned, and divisor must be non-zero.
 *
 * The implementation avoids overflow that can occur with `(dividend + divisor - 1) / divisor`.
 */
template <std::unsigned_integral Dividend, std::unsigned_integral Divisor>
[[nodiscard]] constexpr auto CeilDiv(const Dividend dividend, const Divisor divisor)
{
    assert(divisor > 0);
    const auto quotient{dividend / divisor};
    const auto remainder{dividend % divisor};
    const auto result{quotient + (remainder != 0)};
    if (Assume(result >= quotient)) {
        Assume(result - quotient == static_cast<decltype(result)>(remainder != 0));
    }
    return result;
}

/**
 * @brief Left bit shift with overflow checking.
 * @param input The input value to be left shifted.
 * @param shift The number of bits to left shift.
 * @return (input * 2^shift) or nullopt if it would not fit in the return type.
 */
template <std::integral T>
constexpr std::optional<T> CheckedLeftShift(T input, unsigned shift) noexcept
{
    if (shift == 0 || input == 0) return input;
    // Avoid undefined c++ behaviour if shift is >= number of bits in T.
    if (shift >= sizeof(T) * CHAR_BIT) return std::nullopt;
    // If input << shift is too big to fit in T, return nullopt.
    if (input > (std::numeric_limits<T>::max() >> shift)) return std::nullopt;
    if (input < (std::numeric_limits<T>::min() >> shift)) return std::nullopt;
    return input << shift;
}

/**
 * @brief Left bit shift with safe minimum and maximum values.
 * @param input The input value to be left shifted.
 * @param shift The number of bits to left shift.
 * @return (input * 2^shift) clamped to fit between the lowest and highest
 *         representable values of the type T.
 */
template <std::integral T>
constexpr T SaturatingLeftShift(T input, unsigned shift) noexcept
{
    if (auto result{CheckedLeftShift(input, shift)}) return *result;
    // If input << shift is too big to fit in T, return biggest positive or negative
    // number that fits.
    return input < 0 ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
}

#endif // BITCOIN_UTIL_OVERFLOW_H
