// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H
#define BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H

#include <attributes.h>
#include <crypto/chacha20_vec.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

#ifdef ENABLE_CHACHA20_VEC

namespace chacha20_vec128 {

using vec128 = uint32_t __attribute__((__vector_size__(16)));
static_assert(sizeof(vec128) == 16);

/** Left-rotate vector */
template <size_t BITS>
ALWAYS_INLINE void vec_rotl(vec128& vec)
{
    vec = (vec << BITS) | (vec >> (32 - BITS));
}

#ifdef __SSE2__
template <>
ALWAYS_INLINE void vec_rotl<16>(vec128& vec)
{
    using vec128_u16 = uint16_t __attribute__((__vector_size__(16)));
    const auto halfwords{std::bit_cast<vec128_u16>(vec)};
    vec = std::bit_cast<vec128>(__builtin_shufflevector(halfwords, vec128_u16{}, 1, 0, 3, 2, 5, 4, 7, 6));
}
#endif

#ifdef __SSSE3__
template <>
ALWAYS_INLINE void vec_rotl<8>(vec128& vec)
{
    using vec128_u8 = uint8_t __attribute__((__vector_size__(16)));
    const auto bytes{std::bit_cast<vec128_u8>(vec)};
    vec = std::bit_cast<vec128>(__builtin_shufflevector(bytes, vec128_u8{}, 3, 0, 1, 2, 7, 4, 5, 6, 11, 8, 9, 10, 15, 12, 13, 14));
}
#endif

/** Store a vector in all array elements */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_set_vec128(std::array<vec128, I>& arr, const vec128& vec)
{
    std::get<ITER>(arr) = vec;
    if constexpr (ITER + 1 < I) arr_set_vec128<I, ITER + 1>(arr, vec);
}

/** Add a vector to all array elements */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_vec128(std::array<vec128, I>& arr, const vec128& vec)
{
    std::get<ITER>(arr) += vec;
    if constexpr (ITER + 1 < I) arr_add_vec128<I, ITER + 1>(arr, vec);
}

/** Add corresponding vectors in arr1 to arr0 */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_arr(std::array<vec128, I>& arr0, const std::array<vec128, I>& arr1)
{
    std::get<ITER>(arr0) += std::get<ITER>(arr1);
    if constexpr (ITER + 1 < I) arr_add_arr<I, ITER + 1>(arr0, arr1);
}

} // namespace chacha20_vec128

#endif // ENABLE_CHACHA20_VEC

#endif // BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H
