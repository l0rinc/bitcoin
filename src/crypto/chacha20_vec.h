// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_VEC_H
#define BITCOIN_CRYPTO_CHACHA20_VEC_H

#include <cstddef>
#include <cstdint>

#if defined(__has_attribute) && defined(__has_builtin)
  #if __has_attribute(vector_size) && __has_builtin(__builtin_shufflevector) && (defined(__x86_64__) || defined(__amd64__) || defined(__aarch64__))
    #define ENABLE_CHACHA20_VEC 1
  #endif
#endif

#ifdef ENABLE_CHACHA20_VEC

namespace chacha20_vec {
static constexpr uint16_t BLOCKLEN{64};
static constexpr uint16_t STATE_WORDS{12};
} // namespace chacha20_vec

#endif // ENABLE_CHACHA20_VEC

#endif // BITCOIN_CRYPTO_CHACHA20_VEC_H
