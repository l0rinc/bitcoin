// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define CHACHA20_NAMESPACE chacha20_vec_base

// This file should define which states should be en/disabled for all
// supported architectures. For some, like x86-64 and armv8, simd features
// (sse2 and neon respectively) are safe to use without runtime detection.

#if defined(__x86_64__) || defined(__amd64__)
static constexpr bool CHACHA20_VEC_DISABLE_STATES_16{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_8{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_6{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_4{false};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_2{false};
#elif defined(__ARM_NEON)
static constexpr bool CHACHA20_VEC_DISABLE_STATES_16{false};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_8{false};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_6{false};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_4{false};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_2{true};
#else
// Be conservative and require platforms to opt in.
static constexpr bool CHACHA20_VEC_DISABLE_STATES_16{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_8{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_6{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_4{true};
static constexpr bool CHACHA20_VEC_DISABLE_STATES_2{true};
#endif

#include <crypto/chacha20_vec.ipp>
