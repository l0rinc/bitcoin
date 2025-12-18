// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_PREFETCH_H
#define BITCOIN_UTIL_PREFETCH_H

#include <attributes.h>

#include <cstddef>

namespace util {

ALWAYS_INLINE void Prefetch(const void* ptr) noexcept
{
#if defined(__clang__) || defined(__GNUC__)
    __builtin_prefetch(ptr, 0 /* read */, 0 /* no temporal locality */);
#else
    (void)ptr;
#endif
}

} // namespace util

#endif // BITCOIN_UTIL_PREFETCH_H
