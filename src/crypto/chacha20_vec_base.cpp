// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define CHACHA20_NAMESPACE chacha20_vec_base

// This file should define which states should be en/disabled for all
// supported architectures. For some, like x86-64 and armv8, simd features
// (sse2 and neon respectively) are safe to use without runtime detection.

#if defined(__x86_64__) || defined(__amd64__)
#  define CHACHA20_VEC_DISABLE_STATES_16
#  define CHACHA20_VEC_DISABLE_STATES_8
#  define CHACHA20_VEC_DISABLE_STATES_6
#  if defined(__GNUC__) && !defined(__clang__) && !defined(__AVX2__)
// GCC currently generates slower code for the generic vectorized implementation
// on x86_64 unless AVX2 is enabled. Disable the 4-state path for now to avoid a
// regression.
#    define CHACHA20_VEC_DISABLE_STATES_4
// Disable the 2-state path as well (fallback to scalar) until a faster GCC x86
// implementation exists (e.g. via AVX2/AVX512 runtime dispatch).
#    define CHACHA20_VEC_DISABLE_STATES_2
#  endif
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__)
#  if defined(__GNUC__) && !defined(__clang__)
// The widest multi-state configuration (16) tends to spill on AArch64/NEON.
// Also disable the 6-state variant: it increases code size and hurts the
// common 8/4-state path on this target.
#    define CHACHA20_VEC_DISABLE_STATES_16
#    define CHACHA20_VEC_DISABLE_STATES_6
#    define CHACHA20_VEC_DISABLE_STATES_2
#  else
#    define CHACHA20_VEC_DISABLE_STATES_16
#    define CHACHA20_VEC_DISABLE_STATES_6
#    define CHACHA20_VEC_DISABLE_STATES_2
#  endif
#else
// Be conservative and require platforms to opt-in
#  define CHACHA20_VEC_DISABLE_STATES_16
#  define CHACHA20_VEC_DISABLE_STATES_8
#  define CHACHA20_VEC_DISABLE_STATES_6
#  define CHACHA20_VEC_DISABLE_STATES_4
#  define CHACHA20_VEC_DISABLE_STATES_2
#endif

#include <crypto/chacha20_vec.ipp>
