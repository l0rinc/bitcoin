// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20_vec.h>

#include <bit>
#include <cassert>
#include <cstring>
#include <limits>

#if defined(ENABLE_CHACHA20_VEC)

#if defined(CHACHA20_VEC_DISABLE_STATES_16) && \
    defined(CHACHA20_VEC_DISABLE_STATES_8) && \
    defined(CHACHA20_VEC_DISABLE_STATES_6) && \
    defined(CHACHA20_VEC_DISABLE_STATES_4) && \
    defined(CHACHA20_VEC_DISABLE_STATES_2)
#define CHACHA20_VEC_ALL_MULTI_STATES_DISABLED
#endif


#if !defined(CHACHA20_VEC_ALL_MULTI_STATES_DISABLED)

#if defined(__has_attribute)
#  if __has_attribute(always_inline)
#    define ALWAYS_INLINE __attribute__ ((always_inline)) inline
#  endif
#endif

#if !defined(ALWAYS_INLINE)
#  define ALWAYS_INLINE inline
#endif


namespace {

using vec256 = uint32_t __attribute__((__vector_size__(32)));

// Like Bitcoin Core's `ALWAYS_INLINE` in other files, but kept local to avoid touching shared headers.

/** Endian-conversion for big-endian */
ALWAYS_INLINE void vec_byteswap(vec256& vec)
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

/** Left-rotate vector */
ALWAYS_INLINE void vec_rotl16(vec256& vec)
{
    vec = (vec << 16) | (vec >> 16);
}

ALWAYS_INLINE void vec_rotl12(vec256& vec)
{
    vec = (vec << 12) | (vec >> 20);
}

ALWAYS_INLINE void vec_rotl8(vec256& vec)
{
    vec = (vec << 8) | (vec >> 24);
}

ALWAYS_INLINE void vec_rotl7(vec256& vec)
{
    vec = (vec << 7) | (vec >> 25);
}

static const vec256 nums256 = (vec256){0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};

static const vec256 increments_1[1] = {
    (vec256){1, 0, 0, 0, 0, 0, 0, 0},
};

static const vec256 increments_2[2] = {
    (vec256){1, 0, 0, 0, 0, 0, 0, 0},
    (vec256){3, 0, 0, 0, 2, 0, 0, 0},
};

static const vec256 increments_3[3] = {
    (vec256){1, 0, 0, 0, 0, 0, 0, 0},
    (vec256){3, 0, 0, 0, 2, 0, 0, 0},
    (vec256){5, 0, 0, 0, 4, 0, 0, 0},
};

static const vec256 increments_4[4] = {
    (vec256){1, 0, 0, 0, 0, 0, 0, 0},
    (vec256){3, 0, 0, 0, 2, 0, 0, 0},
    (vec256){5, 0, 0, 0, 4, 0, 0, 0},
    (vec256){7, 0, 0, 0, 6, 0, 0, 0},
};

static const vec256 increments_8[8] = {
    (vec256){1, 0, 0, 0, 0, 0, 0, 0},
    (vec256){3, 0, 0, 0, 2, 0, 0, 0},
    (vec256){5, 0, 0, 0, 4, 0, 0, 0},
    (vec256){7, 0, 0, 0, 6, 0, 0, 0},
    (vec256){9, 0, 0, 0, 8, 0, 0, 0},
    (vec256){11, 0, 0, 0, 10, 0, 0, 0},
    (vec256){13, 0, 0, 0, 12, 0, 0, 0},
    (vec256){15, 0, 0, 0, 14, 0, 0, 0},
};

#define CHACHA20_VEC_PRAGMA(x) _Pragma(#x)
#if defined(__clang__)
#define CHACHA20_VEC_UNROLL(N) CHACHA20_VEC_PRAGMA(clang loop unroll_count(N))
#elif defined(__GNUC__)
#define CHACHA20_VEC_UNROLL(N) CHACHA20_VEC_PRAGMA(GCC unroll N)
#else
#define CHACHA20_VEC_UNROLL(N)
#endif

ALWAYS_INLINE const vec256* increments_for_half_states(size_t half_states)
{
    switch (half_states) {
    case 1: return increments_1;
    case 2: return increments_2;
    case 3: return increments_3;
    case 4: return increments_4;
    case 8: return increments_8;
    default: return nullptr;
    }
}

/** Store a vector in all array elements */
ALWAYS_INLINE void arr_set_vec256(vec256* arr, size_t half_states, const vec256& vec)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr[i] = vec;
    }
}

/** Add a vector to all array elements */
ALWAYS_INLINE void arr_add_vec256(vec256* arr, size_t half_states, const vec256& vec)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr[i] += vec;
    }
}

/** Add corresponding vectors in arr1 to arr0 */
ALWAYS_INLINE void arr_add_arr(vec256* arr0, const vec256* arr1, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr0[i] += arr1[i];
    }
}

ALWAYS_INLINE void arr_add_xor_rot16(vec256* arr0, const vec256* arr1, vec256* arr2, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr0[i] += arr1[i];
        arr2[i] ^= arr0[i];
        vec_rotl16(arr2[i]);
    }
}

ALWAYS_INLINE void arr_add_xor_rot12(vec256* arr0, const vec256* arr1, vec256* arr2, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr0[i] += arr1[i];
        arr2[i] ^= arr0[i];
        vec_rotl12(arr2[i]);
    }
}

ALWAYS_INLINE void arr_add_xor_rot8(vec256* arr0, const vec256* arr1, vec256* arr2, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr0[i] += arr1[i];
        arr2[i] ^= arr0[i];
        vec_rotl8(arr2[i]);
    }
}

ALWAYS_INLINE void arr_add_xor_rot7(vec256* arr0, const vec256* arr1, vec256* arr2, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        arr0[i] += arr1[i];
        arr2[i] ^= arr0[i];
        vec_rotl7(arr2[i]);
    }
}

/*
The first round:
            QUARTERROUND( x0, x4, x8,x12);
            QUARTERROUND( x1, x5, x9,x13);
            QUARTERROUND( x2, x6,x10,x14);
            QUARTERROUND( x3, x7,x11,x15);

The second round:
            QUARTERROUND( x0, x5,x10,x15);
            QUARTERROUND( x1, x6,x11,x12);
            QUARTERROUND( x2, x7, x8,x13);
            QUARTERROUND( x3, x4, x9,x14);

After the first round, arr_shuf0, arr_shuf1, and arr_shuf2 are used to shuffle
the layout to prepare for the second round.

After the second round, they are used (in reverse) to restore the original
layout.

*/
ALWAYS_INLINE void arr_shuf0(vec256* arr, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        vec256& x = arr[i];
        x = __builtin_shufflevector(x, x, 1, 2, 3, 0, 5, 6, 7, 4);
    }
}

ALWAYS_INLINE void arr_shuf1(vec256* arr, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        vec256& x = arr[i];
        x = __builtin_shufflevector(x, x, 2, 3, 0, 1, 6, 7, 4, 5);
    }
}

ALWAYS_INLINE void arr_shuf2(vec256* arr, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        vec256& x = arr[i];
        x = __builtin_shufflevector(x, x, 3, 0, 1, 2, 7, 4, 5, 6);
    }
}

/* Main round function. */
ALWAYS_INLINE void doubleround(vec256* arr0, vec256* arr1, vec256* arr2, vec256* arr3, size_t half_states)
{
    CHACHA20_VEC_UNROLL(10)
    for (size_t i = 0; i < 10; ++i) {
        arr_add_xor_rot16(arr0, arr1, arr3, half_states);
        arr_add_xor_rot12(arr2, arr3, arr1, half_states);
        arr_add_xor_rot8(arr0, arr1, arr3, half_states);
        arr_add_xor_rot7(arr2, arr3, arr1, half_states);
        arr_shuf0(arr1, half_states);
        arr_shuf1(arr2, half_states);
        arr_shuf2(arr3, half_states);
        arr_add_xor_rot16(arr0, arr1, arr3, half_states);
        arr_add_xor_rot12(arr2, arr3, arr1, half_states);
        arr_add_xor_rot8(arr0, arr1, arr3, half_states);
        arr_add_xor_rot7(arr2, arr3, arr1, half_states);
        arr_shuf2(arr1, half_states);
        arr_shuf1(arr2, half_states);
        arr_shuf0(arr3, half_states);
    }
}

/* Read 32bytes of input, xor with calculated state, write to output. Assumes
   that input and output are unaligned, and makes no assumptions about the
   internal layout of vec256;
*/
ALWAYS_INLINE void vec_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256& vec)
{
    assert(in_bytes.size() == 32);
    assert(out_bytes.size() == 32);

    uint32_t temparr[8];
    memcpy(temparr, in_bytes.data(), in_bytes.size());
    vec256 tempvec = vec ^ (vec256){temparr[0], temparr[1], temparr[2], temparr[3], temparr[4], temparr[5], temparr[6], temparr[7]};
    vec_byteswap(tempvec);
    temparr[0] = tempvec[0];
    temparr[1] = tempvec[1];
    temparr[2] = tempvec[2];
    temparr[3] = tempvec[3];
    temparr[4] = tempvec[4];
    temparr[5] = tempvec[5];
    temparr[6] = tempvec[6];
    temparr[7] = tempvec[7];
    memcpy(out_bytes.data(), temparr, out_bytes.size());
}

/* Merge the 128 bit lanes from 2 states to the proper order, then pass each vec_read_xor_write */
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256* arr0, const vec256* arr1, const vec256* arr2, const vec256* arr3, size_t half_states)
{
    for (size_t i = 0; i < half_states; ++i) {
        const vec256& w = arr0[i];
        const vec256& x = arr1[i];
        const vec256& y = arr2[i];
        const vec256& z = arr3[i];

        const size_t offset = i * 128;
        auto in_slice = in_bytes.subspan(offset, 128);
        auto out_slice = out_bytes.subspan(offset, 128);

        vec_read_xor_write(in_slice.first(32), out_slice.first(32), __builtin_shufflevector(w, x, 4, 5, 6, 7, 12, 13, 14, 15));
        vec_read_xor_write(in_slice.subspan(32, 32), out_slice.subspan(32, 32), __builtin_shufflevector(y, z, 4, 5, 6, 7, 12, 13, 14, 15));
        vec_read_xor_write(in_slice.subspan(64, 32), out_slice.subspan(64, 32), __builtin_shufflevector(w, x, 0, 1, 2, 3, 8, 9, 10, 11));
        vec_read_xor_write(in_slice.subspan(96, 32), out_slice.subspan(96, 32), __builtin_shufflevector(y, z, 0, 1, 2, 3, 8, 9, 10, 11));
    }
}

/* Main crypt function. Calculates up to 16 states. */
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256& state0, const vec256& state1, const vec256& state2, size_t states)
{
    const size_t half_states = states / 2;
    const vec256* increments = increments_for_half_states(half_states);
    assert(increments != nullptr);

    vec256 arr0[8], arr1[8], arr2[8], arr3[8];

    arr_set_vec256(arr0, half_states, nums256);
    arr_set_vec256(arr1, half_states, state0);
    arr_set_vec256(arr2, half_states, state1);
    arr_set_vec256(arr3, half_states, state2);

    arr_add_arr(arr3, increments, half_states);

    doubleround(arr0, arr1, arr2, arr3, half_states);

    arr_add_vec256(arr0, half_states, nums256);
    arr_add_vec256(arr1, half_states, state0);
    arr_add_vec256(arr2, half_states, state1);
    arr_add_vec256(arr3, half_states, state2);

    arr_add_arr(arr3, increments, half_states);

    arr_read_xor_write(in_bytes, out_bytes, arr0, arr1, arr2, arr3, half_states);
}

#undef CHACHA20_VEC_UNROLL
#undef CHACHA20_VEC_PRAGMA

} // anonymous namespace
#endif // CHACHA20_VEC_ALL_MULTI_STATES_DISABLED

#if defined(CHACHA20_NAMESPACE)
namespace CHACHA20_NAMESPACE {
#endif

void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const std::array<uint32_t, 12>& input) noexcept
{
#if !defined(CHACHA20_VEC_ALL_MULTI_STATES_DISABLED)
    assert(in_bytes.size() == out_bytes.size());
    const vec256 state0 =  (vec256){input[0], input[1], input[2], input[3], input[0], input[1], input[2], input[3]};
    const vec256 state1 =  (vec256){input[4], input[5], input[6], input[7], input[4], input[5], input[6], input[7]};
    vec256 state2 =  (vec256){input[8], input[9], input[10], input[11], input[8], input[9], input[10], input[11]};
#if !defined(CHACHA20_VEC_DISABLE_STATES_16)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 16) {
        multi_block_crypt(in_bytes, out_bytes, state0, state1, state2, 16);
        state2 += (vec256){16, 0, 0, 0, 16, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 16);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 16);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_8)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 8) {
        multi_block_crypt(in_bytes, out_bytes, state0, state1, state2, 8);
        state2 += (vec256){8, 0, 0, 0, 8, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 8);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 8);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_6)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 6) {
        multi_block_crypt(in_bytes, out_bytes, state0, state1, state2, 6);
        state2 += (vec256){6, 0, 0, 0, 6, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 6);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 6);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_4)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 4) {
        multi_block_crypt(in_bytes, out_bytes, state0, state1, state2, 4);
        state2 += (vec256){4, 0, 0, 0, 4, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 4);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 4);
    }
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_2)
    while(in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * 2) {
        multi_block_crypt(in_bytes, out_bytes, state0, state1, state2, 2);
        state2 += (vec256){2, 0, 0, 0, 2, 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 2);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * 2);
    }
#endif
#endif // CHACHA20_VEC_ALL_MULTI_STATES_DISABLED
}

#if defined(CHACHA20_NAMESPACE)
}
#endif

#endif // ENABLE_CHACHA20_VEC
