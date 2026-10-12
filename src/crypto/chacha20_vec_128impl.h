// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H
#define BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H

#include <attributes.h>
#include <crypto/chacha20_vec.h>
#include <crypto/common.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

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

template <size_t BITS>
ALWAYS_INLINE void vec_add_xor_rot(vec128& x, const vec128& y, vec128& z)
{
    x += y;
    z ^= x;
    vec_rotl<BITS>(z);
}

/** Perform add/xor/rotate for the round function */
template <size_t BITS, size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_xor_rot(std::array<vec128, I>& arr0, const std::array<vec128, I>& arr1, std::array<vec128, I>& arr2)
{
    vec_add_xor_rot<BITS>(std::get<ITER>(arr0), std::get<ITER>(arr1), std::get<ITER>(arr2));

    if constexpr (ITER + 1 < I) arr_add_xor_rot<BITS, I, ITER + 1>(arr0, arr1, arr2);
}

/*
The first round:
            QUARTERROUND(x0, x4, x8, x12);
            QUARTERROUND(x1, x5, x9, x13);
            QUARTERROUND(x2, x6, x10, x14);
            QUARTERROUND(x3, x7, x11, x15);

The second round:
            QUARTERROUND(x0, x5, x10, x15);
            QUARTERROUND(x1, x6, x11, x12);
            QUARTERROUND(x2, x7, x8, x13);
            QUARTERROUND(x3, x4, x9, x14);

After the first round, arr_shuf<1>, arr_shuf<2> and arr_shuf<3> rotate the
lanes to prepare for the second round.

After the second round, the same rotations are applied in reverse to restore
the original layout.

*/

/** Rotate the lanes of every array element left by ROT positions */
template <size_t ROT, size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf(std::array<vec128, I>& arr)
{
    vec128& x = std::get<ITER>(arr);
    x = vec128{x[ROT % 4], x[(ROT + 1) % 4], x[(ROT + 2) % 4], x[(ROT + 3) % 4]};

    if constexpr (ITER + 1 < I) arr_shuf<ROT, I, ITER + 1>(arr);
}

template <size_t I>
ALWAYS_INLINE void doubleround(std::array<vec128, I>& arr0, std::array<vec128, I>& arr1, std::array<vec128, I>& arr2, std::array<vec128, I>& arr3)
{
    for (uint8_t i{0}; i < 10; ++i) {
        arr_add_xor_rot<16>(arr0, arr1, arr3);
        arr_add_xor_rot<12>(arr2, arr3, arr1);
        arr_add_xor_rot<8>(arr0, arr1, arr3);
        arr_add_xor_rot<7>(arr2, arr3, arr1);
        arr_shuf<1>(arr1);
        arr_shuf<2>(arr2);
        arr_shuf<3>(arr3);
        arr_add_xor_rot<16>(arr0, arr1, arr3);
        arr_add_xor_rot<12>(arr2, arr3, arr1);
        arr_add_xor_rot<8>(arr0, arr1, arr3);
        arr_add_xor_rot<7>(arr2, arr3, arr1);
        arr_shuf<3>(arr1);
        arr_shuf<2>(arr2);
        arr_shuf<1>(arr3);
    }
}

/** XOR 16 input bytes with one state row and write them without assuming alignment or vec128's memory layout */
ALWAYS_INLINE void vec_read_xor_write(std::span<const std::byte, 16> in_bytes, std::span<std::byte, 16> out_bytes, const vec128& vec)
{
    const vec128 result{vec ^ vec128{ReadLE32(in_bytes.data()), ReadLE32(in_bytes.data() + 4), ReadLE32(in_bytes.data() + 8), ReadLE32(in_bytes.data() + 12)}};
    WriteLE32(out_bytes.data(), result[0]);
    WriteLE32(out_bytes.data() + 4, result[1]);
    WriteLE32(out_bytes.data() + 8, result[2]);
    WriteLE32(out_bytes.data() + 12, result[3]);
}

/** Write each 64-byte state in row order */
template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const std::array<vec128, I>& arr0, const std::array<vec128, I>& arr1, const std::array<vec128, I>& arr2, const std::array<vec128, I>& arr3)
{
    const vec128& w = std::get<ITER>(arr0);
    const vec128& x = std::get<ITER>(arr1);
    const vec128& y = std::get<ITER>(arr2);
    const vec128& z = std::get<ITER>(arr3);

    vec_read_xor_write(in_bytes.first<16>(), out_bytes.first<16>(), w);
    vec_read_xor_write(in_bytes.subspan<16, 16>(), out_bytes.subspan<16, 16>(), x);
    vec_read_xor_write(in_bytes.subspan<32, 16>(), out_bytes.subspan<32, 16>(), y);
    vec_read_xor_write(in_bytes.subspan<48, 16>(), out_bytes.subspan<48, 16>(), z);

    if constexpr (ITER + 1 < I) arr_read_xor_write<I, ITER + 1>(in_bytes.subspan<64>(), out_bytes.subspan<64>(), arr0, arr1, arr2, arr3);
}

template <size_t SIZE>
consteval std::array<vec128, SIZE> generate_increments()
{
    std::array<vec128, SIZE> rows;
    for (uint32_t i{0}; i < SIZE; ++i) {
        rows[i] = vec128{i, 0, 0, 0};
    }
    return rows;
}

template <size_t STATES>
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec128& state0, const vec128& state1, const vec128& state2)
{
    static constexpr vec128 constants{0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    static constinit const std::array<vec128, STATES> increments{generate_increments<STATES>()};
    std::array<vec128, STATES> arr0, arr1, arr2, arr3;

    arr_set_vec128(arr0, constants);
    arr_set_vec128(arr1, state0);
    arr_set_vec128(arr2, state1);
    arr_set_vec128(arr3, state2);
    arr_add_arr(arr3, increments);

    doubleround(arr0, arr1, arr2, arr3);

    arr_add_vec128(arr0, constants);
    arr_add_vec128(arr1, state0);
    arr_add_vec128(arr2, state1);
    arr_add_vec128(arr3, state2);
    arr_add_arr(arr3, increments);

    arr_read_xor_write(in_bytes, out_bytes, arr0, arr1, arr2, arr3);
}

class ChaCha20Vectorized
{
    const vec128 state0;
    const vec128 state1;
    vec128 state2;

public:
    explicit ALWAYS_INLINE ChaCha20Vectorized(std::span<const uint32_t, chacha20_vec::STATE_WORDS> input) noexcept
        : state0{input[0], input[1], input[2], input[3]}
        , state1{input[4], input[5], input[6], input[7]}
        , state2{input[8], input[9], input[10], input[11]}
    {
    }
    template <size_t STATES>
    ALWAYS_INLINE void CryptStates(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes)
    {
        multi_block_crypt<STATES>(in_bytes, out_bytes, state0, state1, state2);
        state2 += vec128{STATES, 0, 0, 0};
        in_bytes = in_bytes.subspan(chacha20_vec::BLOCKLEN * STATES);
        out_bytes = out_bytes.subspan(chacha20_vec::BLOCKLEN * STATES);
    }
};

} // namespace chacha20_vec128

#endif // ENABLE_CHACHA20_VEC

#endif // BITCOIN_CRYPTO_CHACHA20_VEC_128IMPL_H
