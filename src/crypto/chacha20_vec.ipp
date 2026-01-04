// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <attributes.h>
#include <crypto/chacha20_vec.h>

#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <utility>

#if defined(ENABLE_CHACHA20_VEC)

// Convert preprocessor flags to constexpr booleans for use with if constexpr
#ifdef CHACHA20_VEC_DISABLE_STATES_16
inline constexpr bool kEnableStates16 = false;
#else
inline constexpr bool kEnableStates16 = true;
#endif

#ifdef CHACHA20_VEC_DISABLE_STATES_8
inline constexpr bool kEnableStates8 = false;
#else
inline constexpr bool kEnableStates8 = true;
#endif

#ifdef CHACHA20_VEC_DISABLE_STATES_6
inline constexpr bool kEnableStates6 = false;
#else
inline constexpr bool kEnableStates6 = true;
#endif

#ifdef CHACHA20_VEC_DISABLE_STATES_4
inline constexpr bool kEnableStates4 = false;
#else
inline constexpr bool kEnableStates4 = true;
#endif

#ifdef CHACHA20_VEC_DISABLE_STATES_2
inline constexpr bool kEnableStates2 = false;
#else
inline constexpr bool kEnableStates2 = true;
#endif

inline constexpr bool kEnableAnyMultiState = kEnableStates16 || kEnableStates8 || kEnableStates6 || kEnableStates4 || kEnableStates2;

// vec256 type must be visible for if constexpr branches even when they're not taken
using vec256 = uint32_t __attribute__((__vector_size__(32)));

// Preprocessor check for conditional compilation of the anonymous namespace
#if !defined(CHACHA20_VEC_DISABLE_STATES_16) || \
    !defined(CHACHA20_VEC_DISABLE_STATES_8) ||  \
    !defined(CHACHA20_VEC_DISABLE_STATES_6) ||  \
    !defined(CHACHA20_VEC_DISABLE_STATES_4) ||  \
    !defined(CHACHA20_VEC_DISABLE_STATES_2)

namespace {

// Used for an optional aligned I/O fast-path.
static constexpr size_t CHACHA20_VEC_MEM_ALIGN{16};

/** Endian-conversion for big-endian */
ALWAYS_INLINE void vec_byteswap(vec256& vec)
{
    if constexpr (std::endian::native == std::endian::big) {
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

/** Left-rotate all elements in a vector by N bits */
template <unsigned N>
ALWAYS_INLINE void vec_rotl(vec256& vec)
{
    static_assert(N > 0 && N < 32, "Rotation must be between 1 and 31 bits");
    vec = (vec << N) | (vec >> (32 - N));
}

static constexpr vec256 nums256 = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};

// Counter increments for each half-state pair. Pattern: {2*i+1, 0, 0, 0, 2*i, 0, 0, 0}
// All smaller state counts use a prefix of this array.
static constexpr vec256 increments[8] = {
    {1, 0, 0, 0, 0, 0, 0, 0},
    {3, 0, 0, 0, 2, 0, 0, 0},
    {5, 0, 0, 0, 4, 0, 0, 0},
    {7, 0, 0, 0, 6, 0, 0, 0},
    {9, 0, 0, 0, 8, 0, 0, 0},
    {11, 0, 0, 0, 10, 0, 0, 0},
    {13, 0, 0, 0, 12, 0, 0, 0},
    {15, 0, 0, 0, 14, 0, 0, 0},
};

template <typename Fn, size_t... I>
ALWAYS_INLINE void for_each_half_state(size_t half_states, Fn&& fn, std::index_sequence<I...>)
{
    ((I < half_states ? (fn(std::integral_constant<size_t, I>{}), 0) : 0), ...);
}

template <typename Fn>
ALWAYS_INLINE void for_each_half_state(size_t half_states, Fn&& fn)
{
    for_each_half_state(half_states, std::forward<Fn>(fn), std::make_index_sequence<8>{});
}

/** Store a vector in all array elements */
ALWAYS_INLINE void arr_set_vec256(vec256* arr, size_t half_states, const vec256& vec)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        arr[i] = vec;
    });
}

/** Add a vector to all array elements */
ALWAYS_INLINE void arr_add_vec256(vec256* arr, size_t half_states, const vec256& vec)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        arr[i] += vec;
    });
}

/** Add corresponding vectors in arr1 to arr0 */
ALWAYS_INLINE void arr_add_arr(vec256* arr0, const vec256* arr1, size_t half_states)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        arr0[i] += arr1[i];
    });
}

/** Add arr1 to arr0, XOR result into arr2, rotate arr2 left by N bits */
template <unsigned N>
ALWAYS_INLINE void arr_add_xor_rot(vec256* arr0, const vec256* arr1, vec256* arr2, size_t half_states)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        arr0[i] += arr1[i];
        arr2[i] ^= arr0[i];
        vec_rotl<N>(arr2[i]);
    });
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
#define VEC_SHUF_SELF(x, ...) __builtin_shufflevector(x, x, __VA_ARGS__)
#define VEC_SHUF2(a, b, ...) __builtin_shufflevector(a, b, __VA_ARGS__)

ALWAYS_INLINE void arr_shuf0(vec256* arr, size_t half_states)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        vec256& x = arr[i];
        x = VEC_SHUF_SELF(x, 1, 2, 3, 0, 5, 6, 7, 4);
    });
}

ALWAYS_INLINE void arr_shuf1(vec256* arr, size_t half_states)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        vec256& x = arr[i];
        x = VEC_SHUF_SELF(x, 2, 3, 0, 1, 6, 7, 4, 5);
    });
}

ALWAYS_INLINE void arr_shuf2(vec256* arr, size_t half_states)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;
        vec256& x = arr[i];
        x = VEC_SHUF_SELF(x, 3, 0, 1, 2, 7, 4, 5, 6);
    });
}

/* Main round function. */
ALWAYS_INLINE void doubleround(vec256* arr0, vec256* arr1, vec256* arr2, vec256* arr3, size_t half_states)
{
    UNROLL_LOOP(10)
    for (size_t i = 0; i < 10; ++i) {
        arr_add_xor_rot<16>(arr0, arr1, arr3, half_states);
        arr_add_xor_rot<12>(arr2, arr3, arr1, half_states);
        arr_add_xor_rot<8>(arr0, arr1, arr3, half_states);
        arr_add_xor_rot<7>(arr2, arr3, arr1, half_states);
        arr_shuf0(arr1, half_states);
        arr_shuf1(arr2, half_states);
        arr_shuf2(arr3, half_states);
        arr_add_xor_rot<16>(arr0, arr1, arr3, half_states);
        arr_add_xor_rot<12>(arr2, arr3, arr1, half_states);
        arr_add_xor_rot<8>(arr0, arr1, arr3, half_states);
        arr_add_xor_rot<7>(arr2, arr3, arr1, half_states);
        arr_shuf2(arr1, half_states);
        arr_shuf1(arr2, half_states);
        arr_shuf0(arr3, half_states);
    }
}

/* Read 32 bytes of input, xor with calculated state, write to output.
   Supports unaligned input/output, with an optional aligned fast-path.
*/
template <bool AssumeAligned>
ALWAYS_INLINE void vec_read_xor_write(const std::byte* in_bytes, std::byte* out_bytes, const vec256& vec)
{
    if constexpr (AssumeAligned) {
        in_bytes = std::assume_aligned<CHACHA20_VEC_MEM_ALIGN>(in_bytes);
        out_bytes = std::assume_aligned<CHACHA20_VEC_MEM_ALIGN>(out_bytes);
    }

    uint32_t tmp_arr[8];
    memcpy(tmp_arr, in_bytes, sizeof(tmp_arr));
    vec256 tmp_vec;
    memcpy(&tmp_vec, tmp_arr, sizeof(tmp_vec));
    vec_byteswap(tmp_vec);

    tmp_vec ^= vec;
    vec_byteswap(tmp_vec);

    memcpy(out_bytes, &tmp_vec, sizeof(tmp_vec));
}

template <bool AssumeAligned>
ALWAYS_INLINE void arr_read_xor_write_impl(const std::byte* in_bytes, std::byte* out_bytes, const vec256* arr0, const vec256* arr1, const vec256* arr2, const vec256* arr3, size_t half_states)
{
    for_each_half_state(half_states, [&](auto idx) {
        constexpr size_t i = decltype(idx)::value;

        const vec256& w = arr0[i];
        const vec256& x = arr1[i];
        const vec256& y = arr2[i];
        const vec256& z = arr3[i];

        const size_t offset = i * 128;
        const std::byte* in_slice = in_bytes + offset;
        std::byte* out_slice = out_bytes + offset;

        vec_read_xor_write<AssumeAligned>(in_slice + 0, out_slice + 0, VEC_SHUF2(w, x, 4, 5, 6, 7, 12, 13, 14, 15));
        vec_read_xor_write<AssumeAligned>(in_slice + 32, out_slice + 32, VEC_SHUF2(y, z, 4, 5, 6, 7, 12, 13, 14, 15));
        vec_read_xor_write<AssumeAligned>(in_slice + 64, out_slice + 64, VEC_SHUF2(w, x, 0, 1, 2, 3, 8, 9, 10, 11));
        vec_read_xor_write<AssumeAligned>(in_slice + 96, out_slice + 96, VEC_SHUF2(y, z, 0, 1, 2, 3, 8, 9, 10, 11));
    });
}

/* Merge the 128 bit lanes from 2 states to the proper order, then pass each vec_read_xor_write */
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256* arr0, const vec256* arr1, const vec256* arr2, const vec256* arr3, size_t half_states)
{
    constexpr std::uintptr_t mask{CHACHA20_VEC_MEM_ALIGN - 1};
    const bool aligned = ((reinterpret_cast<std::uintptr_t>(in_bytes.data()) | reinterpret_cast<std::uintptr_t>(out_bytes.data())) & mask) == 0;

    if (aligned) [[likely]] {
        arr_read_xor_write_impl<true>(in_bytes.data(), out_bytes.data(), arr0, arr1, arr2, arr3, half_states);
    } else {
        arr_read_xor_write_impl<false>(in_bytes.data(), out_bytes.data(), arr0, arr1, arr2, arr3, half_states);
    }
}

/* Main crypt function. Calculates up to 16 states (8 half_states). */
template <size_t States>
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256& state0, const vec256& state1, const vec256& state2)
{
    static_assert(States == 16 || States == 8 || States == 6 || States == 4 || States == 2);
    constexpr size_t half_states = States / 2;
    vec256 arr0[half_states], arr1[half_states], arr2[half_states], arr3[half_states];

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

template <size_t States>
ALWAYS_INLINE void process_blocks(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const vec256& state0, const vec256& state1, vec256& state2)
{
    while (in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * States) {
        multi_block_crypt<States>(in_bytes, out_bytes, state0, state1, state2);
        state2 += (vec256){static_cast<uint32_t>(States), 0, 0, 0, static_cast<uint32_t>(States), 0, 0, 0};
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * States);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * States);
    }
}

#undef VEC_SHUF_SELF
#undef VEC_SHUF2

} // anonymous namespace
#endif // any multi-state enabled

#if defined(CHACHA20_NAMESPACE)
namespace CHACHA20_NAMESPACE {
#endif

void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const std::array<uint32_t, 12>& input) noexcept
{
    if constexpr (kEnableAnyMultiState) {
        assert(in_bytes.size() == out_bytes.size());
        const vec256 state0 = (vec256){input[0], input[1], input[2], input[3], input[0], input[1], input[2], input[3]};
        const vec256 state1 = (vec256){input[4], input[5], input[6], input[7], input[4], input[5], input[6], input[7]};
        vec256 state2 = (vec256){input[8], input[9], input[10], input[11], input[8], input[9], input[10], input[11]};

        if constexpr (kEnableStates16) process_blocks<16>(in_bytes, out_bytes, state0, state1, state2);
        if constexpr (kEnableStates8) process_blocks<8>(in_bytes, out_bytes, state0, state1, state2);
        if constexpr (kEnableStates6) process_blocks<6>(in_bytes, out_bytes, state0, state1, state2);
        if constexpr (kEnableStates4) process_blocks<4>(in_bytes, out_bytes, state0, state1, state2);
        if constexpr (kEnableStates2) process_blocks<2>(in_bytes, out_bytes, state0, state1, state2);
    }
}

#if defined(CHACHA20_NAMESPACE)
}
#endif

#endif // ENABLE_CHACHA20_VEC
