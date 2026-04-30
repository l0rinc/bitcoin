// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_VEC_H
#define BITCOIN_CRYPTO_CHACHA20_VEC_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

static constexpr size_t CHACHA20_VEC_BLOCKLEN{64};

#if !defined(DISABLE_CHACHA20_VEC) && defined(__has_builtin)
#  if __has_builtin(__builtin_shufflevector)
#    define ENABLE_CHACHA20_VEC 1
#  endif
#endif

#if defined(ENABLE_CHACHA20_VEC)
namespace chacha20_vec_base {
void Crypt(std::span<const std::byte>& input, std::span<std::byte>& output, const std::array<uint32_t, 12>& state) noexcept;
} // namespace chacha20_vec_base
#endif

#endif // BITCOIN_CRYPTO_CHACHA20_VEC_H
