// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_FOR_EACH_INDEX_H
#define BITCOIN_UTIL_FOR_EACH_INDEX_H

#include <attributes.h>

#include <cstddef>
#include <utility>

namespace util {

/** Invoke `fn.template operator()<I>()` for each `I` in `[0, N)`. */
template <size_t N, typename Fn, size_t... I>
ALWAYS_INLINE void ForEachIndex(Fn&& fn, std::index_sequence<I...>)
{
    (fn.template operator()<I>(), ...);
}

template <size_t N, typename Fn>
ALWAYS_INLINE void ForEachIndex(Fn&& fn)
{
    ForEachIndex<N>(std::forward<Fn>(fn), std::make_index_sequence<N>{});
}

} // namespace util

#endif // BITCOIN_UTIL_FOR_EACH_INDEX_H
