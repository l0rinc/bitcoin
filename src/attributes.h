// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ATTRIBUTES_H
#define BITCOIN_ATTRIBUTES_H

#if defined(__clang__)
#  if __has_attribute(lifetimebound)
#    define LIFETIMEBOUND [[clang::lifetimebound]]
#  else
#    define LIFETIMEBOUND
#  endif
#else
#  define LIFETIMEBOUND
#endif

#if defined(__GNUC__) // Covers GCC and Clang
#  define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#  define ALWAYS_INLINE __forceinline
#else
#  error No known always_inline attribute for this platform.
#endif

#if defined(__GNUC__) && !defined(__clang__)
#  define FORCE_O3 [[gnu::optimize("-O3")]]
#  define OPTIMIZED(func_def) \
FORCE_O3 func_def
#elif defined(__clang__)
#  define FORCE_O3
#  define OPTIMIZED(func_def) \
_Pragma("clang optimize on") \
func_def \
_Pragma("clang optimize off")
#else
#  define FORCE_O3
#  define OPTIMIZED(func_def) func_def
#endif

#endif // BITCOIN_ATTRIBUTES_H
