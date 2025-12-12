// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20_vec.h>

#include <array>
#include <cassert>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <bit>
#include <limits>
#include <span>

#ifdef ENABLE_CHACHA20_VEC

#if defined __has_attribute
#  if __has_attribute (always_inline)
#    define ALWAYS_INLINE __attribute__ ((always_inline)) inline
#  else
#    define ALWAYS_INLINE inline
#  endif
#endif


namespace {

using vec256 = uint32_t __attribute__((__vector_size__(32)));

ALWAYS_INLINE void byteswap_vec(vec256& vec)
{
    if constexpr (std::endian::native == std::endian::big)
    {
        vec256 ret;
        ret[0] = __builtin_bswap32(vec[0]);
        ret[1] = __builtin_bswap32(vec[1]);
        ret[2] = __builtin_bswap32(vec[2]);
        ret[3] = __builtin_bswap32(vec[3]);
        ret[4] = __builtin_bswap32(vec[4]);
        ret[5] = __builtin_bswap32(vec[5]);
        ret[6] = __builtin_bswap32(vec[6]);
        ret[7] = __builtin_bswap32(vec[7]);
        vec = ret;
    }
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_set_vec256(std::array<vec256, I>& arr0, const vec256& y)
{
    std::get<ITER>(arr0) = y;
    if constexpr(ITER + 1 < I ) arr_set_vec256<I, ITER + 1>(arr0, y);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_vec256(std::array<vec256, I>& arr0, const vec256& y)
{
    std::get<ITER>(arr0) += y;
    if constexpr(ITER + 1 < I ) arr_add_vec256<I, ITER + 1>(arr0, y);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_arr(std::array<vec256, I>& arr0, const std::array<vec256, I>& arr1)
{
    std::get<ITER>(arr0) += std::get<ITER>(arr1);
    if constexpr(ITER + 1 < I ) arr_add_arr<I, ITER + 1>(arr0, arr1);
}

template <size_t BITS>
ALWAYS_INLINE void rotl(vec256& x)
{
    x = (x << BITS) | (x >> (32 - BITS));
}

ALWAYS_INLINE void vec_unshuffle(vec256& w, vec256& x, vec256& y, vec256& z)
{
    const vec256 wtemp = w;
    const vec256 xtemp = x;
    const vec256 ytemp = y;
    const vec256 ztemp = z;

    w = __builtin_shufflevector(wtemp, xtemp, 4, 5, 6, 7, 12, 13, 14, 15);
    x = __builtin_shufflevector(ytemp, ztemp, 4, 5, 6, 7, 12, 13, 14, 15);
    y = __builtin_shufflevector(wtemp, xtemp, 0, 1, 2, 3, 8, 9, 10, 11);
    z = __builtin_shufflevector(ytemp, ztemp, 0, 1, 2, 3, 8, 9, 10, 11);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_unshuffle(std::array<vec256, I>& arr0, std::array<vec256, I>& arr1, std::array<vec256, I>& arr2, std::array<vec256, I>& arr3)
{
    vec_unshuffle(std::get<ITER>(arr0), std::get<ITER>(arr1), std::get<ITER>(arr2), std::get<ITER>(arr3));
    if constexpr(ITER + 1 < I ) arr_unshuffle<I, ITER + 1>(arr0, arr1, arr2, arr3);
}

template <size_t BITS, size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_add_xor_rot(std::array<vec256, I>& arr0, const std::array<vec256, I>& arr1, std::array<vec256, I>& arr2)
{
    vec256& x = std::get<ITER>(arr0);
    const vec256& y = std::get<ITER>(arr1);
    vec256& z = std::get<ITER>(arr2);

    x += y;
    z ^= x;
    rotl<BITS>(z);

    if constexpr(ITER + 1 < I ) arr_add_xor_rot<BITS, I, ITER + 1>(arr0, arr1, arr2);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf0(std::array<vec256, I>& arr0)
{
    vec256& x = std::get<ITER>(arr0);
    x = __builtin_shufflevector(x, x, 1, 2, 3, 0, 5, 6, 7, 4);
    if constexpr(ITER + 1 < I ) arr_shuf0<I, ITER + 1>(arr0);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf1(std::array<vec256, I>& arr0)
{
    vec256& x = std::get<ITER>(arr0);
    x = __builtin_shufflevector(x, x, 2, 3, 0, 1, 6, 7, 4, 5);
    if constexpr(ITER + 1 < I ) arr_shuf1<I, ITER + 1>(arr0);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_shuf2(std::array<vec256, I>& arr0)
{
    vec256& x = std::get<ITER>(arr0);
    x = __builtin_shufflevector(x, x, 3, 0, 1, 2, 7, 4, 5, 6);
    if constexpr(ITER + 1 < I ) arr_shuf2<I, ITER + 1>(arr0);
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void doubleround(std::array<vec256, I>& arr0, std::array<vec256, I>& arr1, std::array<vec256, I>&arr2, std::array<vec256, I>&arr3)
{
    arr_add_xor_rot<16>(arr0, arr1, arr3);
    arr_add_xor_rot<12>(arr2, arr3, arr1);
    arr_add_xor_rot<8>(arr0, arr1, arr3);
    arr_add_xor_rot<7>(arr2, arr3, arr1);
    arr_shuf0(arr1);
    arr_shuf1(arr2);
    arr_shuf2(arr3);
    arr_add_xor_rot<16>(arr0, arr1, arr3);
    arr_add_xor_rot<12>(arr2, arr3, arr1);
    arr_add_xor_rot<8>(arr0, arr1, arr3);
    arr_add_xor_rot<7>(arr2, arr3, arr1);
    arr_shuf2(arr1);
    arr_shuf1(arr2);
    arr_shuf0(arr3);
    if constexpr (ITER + 1 < 10) doubleround<I, ITER + 1>(arr0, arr1, arr2, arr3);
}


ALWAYS_INLINE
void vec_read_xor_write(std::span<const std::byte, 32> in_bytes, std::span<std::byte, 32> out_bytes, const vec256& vec)
{
    // Post-c++-23, this can be: uint32_t temparr[8] = start_lifetime_as_array(in_bytes.data(), in_bytes.size());
    std::array<uint32_t, 8> temparr;
    memcpy(temparr.data(), in_bytes.data(), in_bytes.size());
    vec256 tempvec = vec ^ (vec256){temparr[0], temparr[1], temparr[2], temparr[3], temparr[4], temparr[5], temparr[6], temparr[7]};
    byteswap_vec(tempvec);
    temparr = {tempvec[0], tempvec[1], tempvec[2], tempvec[3], tempvec[4], tempvec[5], tempvec[6], tempvec[7]};
    memcpy(out_bytes.data(), temparr.data(), out_bytes.size());
}

template <size_t I, size_t ITER = 0>
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const std::array<vec256, I>& arr0, const std::array<vec256, I>& arr1, const std::array<vec256, I>& arr2, const std::array<vec256, I>& arr3)
{
    vec_read_xor_write(in_bytes.first<32>(), out_bytes.first<32>(), std::get<ITER>(arr0));
    vec_read_xor_write(in_bytes.subspan<32, 32>(), out_bytes.subspan<32, 32>(), std::get<ITER>(arr1));
    vec_read_xor_write(in_bytes.subspan<64, 32>(), out_bytes.subspan<64, 32>(), std::get<ITER>(arr2));
    vec_read_xor_write(in_bytes.subspan<96, 32>(), out_bytes.subspan<96, 32>(), std::get<ITER>(arr3));

    if constexpr(ITER + 1 < I ) arr_read_xor_write<I, ITER + 1>(in_bytes.subspan<128>(), out_bytes.subspan<128>(), arr0, arr1, arr2, arr3);
}

template <size_t SIZE>
consteval std::array<vec256, SIZE> generate_rows()
{
    // Generate vectors of the pattern:
    // 1 0 0 0 0 0 0 0
    // 3 0 0 0 2 0 0 0
    // 5 0 0 0 4 0 0 0
    // ...
    std::array<vec256, SIZE> rows;
    for (uint32_t i = 0; i < SIZE; i ++)
    {
        rows[i] = (i * (vec256){2, 0, 0, 0, 2, 0, 0, 0}) + (vec256){1, 0, 0, 0, 0, 0, 0, 0};
    }
    return rows;
}

template <size_t STATES>
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256& state0, const vec256& state1, const vec256& state2)
{
    static constexpr size_t HALF_STATES = STATES / 2;
    static constexpr vec256 nums256 = (vec256){0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    static constinit std::array<vec256, HALF_STATES> rows = generate_rows<HALF_STATES>();

    std::array<vec256, HALF_STATES> arr0, arr1, arr2, arr3;

    arr_set_vec256(arr0, nums256);
    arr_set_vec256(arr1, state0);
    arr_set_vec256(arr2, state1);
    arr_set_vec256(arr3, state2);

    arr_add_arr(arr3, rows);

    doubleround(arr0, arr1, arr2, arr3);

    arr_add_vec256(arr0, nums256);
    arr_add_vec256(arr1, state0);
    arr_add_vec256(arr2, state1);
    arr_add_vec256(arr3, state2);

    arr_add_arr(arr3, rows);

    arr_unshuffle(arr0, arr1, arr2, arr3);
    arr_read_xor_write(in_bytes, out_bytes, arr0, arr1, arr2, arr3);
}

} // anonymous namespace

#if defined(CHACHA20_NAMESPACE)
namespace CHACHA20_NAMESPACE {
#endif

size_t chacha20_crypt_vectorized(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const std::array<uint32_t, 12>& input) noexcept
{
    assert(in_bytes.size() == out_bytes.size());
    const size_t blocks_in = out_bytes.size() / CHACHA20_VEC_BLOCKLEN;
    size_t blocks = blocks_in;
    assert(blocks * CHACHA20_VEC_BLOCKLEN == out_bytes.size());

    const vec256 state0 =  (vec256){input[0], input[1], input[2], input[3], input[0], input[1], input[2], input[3]};
    const vec256 state1 =  (vec256){input[4], input[5], input[6], input[7], input[4], input[5], input[6], input[7]};
    vec256 state2 =  (vec256){input[8], input[9], input[10], input[11], input[8], input[9], input[10], input[11]};
#if !defined(CHACHA20_VEC_DISABLE_STATES_16)
    while(blocks >= 16) {
        multi_block_crypt<16>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){16, 0, 0, 0, 16, 0, 0, 0};
        blocks -= 16;
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 16);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 16);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_8)
    while(blocks >= 8) {
        multi_block_crypt<8>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){8, 0, 0, 0, 8, 0, 0, 0};
        blocks -= 8;
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 8);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 8);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_6)
    while(blocks >= 6) {
        multi_block_crypt<6>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){6, 0, 0, 0, 6, 0, 0, 0};
        blocks -= 6;
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 6);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 6);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_4)
    while(blocks >= 4) {
        multi_block_crypt<4>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){4, 0, 0, 0, 4, 0, 0, 0};
        blocks -= 4;
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 4);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 4);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_2)
    while(blocks >= 2) {
        multi_block_crypt<2>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){2, 0, 0, 0, 2, 0, 0, 0};
        blocks -= 2;
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 2);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 2);
    }
#endif
    return blocks_in - blocks;
}

#if defined(CHACHA20_NAMESPACE)
}
#endif

#endif // ENABLE_CHACHA20_VEC
