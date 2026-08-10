// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_BYTE_UNITS_H
#define BITCOIN_UTIL_BYTE_UNITS_H

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace util::detail {
template <uint64_t BYTES_PER_UNIT>
consteval uint64_t ByteUnitsToBytes(unsigned long long units)
{
    static_assert(BYTES_PER_UNIT > 0);
    if (units > std::numeric_limits<uint64_t>::max() / BYTES_PER_UNIT) {
        throw std::overflow_error("Too large");
    }
    return units * BYTES_PER_UNIT;
}
} // namespace util::detail

/// Conversion of MiB to bytes.
consteval uint64_t operator""_MiB(unsigned long long mebibytes)
{
    return util::detail::ByteUnitsToBytes<uint64_t{1} << 20>(mebibytes);
}

/// Conversion of GiB to bytes.
consteval uint64_t operator""_GiB(unsigned long long gibibytes)
{
    return util::detail::ByteUnitsToBytes<uint64_t{1} << 30>(gibibytes);
}

/// Conversion of MB to bytes.
consteval uint64_t operator""_MB(unsigned long long megabytes)
{
    return util::detail::ByteUnitsToBytes<1'000'000>(megabytes);
}

/// Conversion of GB to bytes.
consteval uint64_t operator""_GB(unsigned long long gigabytes)
{
    return util::detail::ByteUnitsToBytes<1'000'000'000>(gigabytes);
}

#endif // BITCOIN_UTIL_BYTE_UNITS_H
