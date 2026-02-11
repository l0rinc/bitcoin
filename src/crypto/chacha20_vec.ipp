// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <attributes.h>
#include <crypto/chacha20_vec.h>
#include <util/for_each_index.h>

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

using vec128 = uint32_t __attribute__((__vector_size__(16)));

#if defined(__GNUC__) && !defined(__clang__)
#  define CHACHA20_VEC_USE_SPLIT_LANES 1
#else
#  define CHACHA20_VEC_USE_SPLIT_LANES 0
#endif

#if CHACHA20_VEC_USE_SPLIT_LANES
// Represent two 128-bit lanes explicitly. This avoids GCC generating expensive
// scalar sequences for 256-bit shuffles on targets without native 256-bit SIMD
// registers (e.g. AArch64/NEON, x86/SSE2).
struct vec256 {
    vec128 lo;
    vec128 hi;
};
static_assert(sizeof(vec256) == 32);

ALWAYS_INLINE vec256& operator+=(vec256& a, const vec256& b)
{
    a.lo += b.lo;
    a.hi += b.hi;
    return a;
}

ALWAYS_INLINE vec256& operator^=(vec256& a, const vec256& b)
{
    a.lo ^= b.lo;
    a.hi ^= b.hi;
    return a;
}
#else
using vec256 = uint32_t __attribute__((__vector_size__(32)));
#endif

// Preprocessor check for conditional compilation of the anonymous namespace and
// the multi-state code paths. When all states are disabled, avoid referencing
// templates/functions that are not available.
#if !defined(CHACHA20_VEC_DISABLE_STATES_16) || \
    !defined(CHACHA20_VEC_DISABLE_STATES_8) ||  \
    !defined(CHACHA20_VEC_DISABLE_STATES_6) ||  \
    !defined(CHACHA20_VEC_DISABLE_STATES_4) ||  \
    !defined(CHACHA20_VEC_DISABLE_STATES_2)
#  define CHACHA20_VEC_ENABLE_ANY_MULTI_STATE 1
#else
#  define CHACHA20_VEC_ENABLE_ANY_MULTI_STATE 0
#endif

#if CHACHA20_VEC_ENABLE_ANY_MULTI_STATE

namespace {

// Used for an optional aligned I/O fast-path.
static constexpr size_t CHACHA20_VEC_MEM_ALIGN{16};

/** Endian-conversion for big-endian */
ALWAYS_INLINE void vec_byteswap(vec256& vec)
{
    if constexpr (std::endian::native == std::endian::big) {
#if CHACHA20_VEC_USE_SPLIT_LANES
        vec128 lo;
        lo[0] = __builtin_bswap32(vec.lo[0]);
        lo[1] = __builtin_bswap32(vec.lo[1]);
        lo[2] = __builtin_bswap32(vec.lo[2]);
        lo[3] = __builtin_bswap32(vec.lo[3]);
        vec128 hi;
        hi[0] = __builtin_bswap32(vec.hi[0]);
        hi[1] = __builtin_bswap32(vec.hi[1]);
        hi[2] = __builtin_bswap32(vec.hi[2]);
        hi[3] = __builtin_bswap32(vec.hi[3]);
        vec.lo = lo;
        vec.hi = hi;
#else
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
#endif
    }
}

/** Left-rotate all elements in a vector by N bits */
template <unsigned N>
ALWAYS_INLINE void vec_rotl(vec256& vec)
{
    static_assert(N > 0 && N < 32, "Rotation must be between 1 and 31 bits");
#if CHACHA20_VEC_USE_SPLIT_LANES
    vec.lo = (vec.lo << N) | (vec.lo >> (32 - N));
    vec.hi = (vec.hi << N) | (vec.hi >> (32 - N));
#else
    vec = (vec << N) | (vec >> (32 - N));
#endif
}

ALWAYS_INLINE void vec_add_counter(vec256& vec, uint32_t inc)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    vec += vec256{{inc, 0, 0, 0}, {inc, 0, 0, 0}};
#else
    vec += (vec256){inc, 0, 0, 0, inc, 0, 0, 0};
#endif
}

#if CHACHA20_VEC_USE_SPLIT_LANES
static constexpr vec128 nums128 = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
static constexpr vec256 nums256 = {nums128, nums128};
#else
static constexpr vec256 nums256 = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
#endif

ALWAYS_INLINE vec256 vec_broadcast4(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    const vec128 lane = {a, b, c, d};
    return vec256{lane, lane};
#else
    return (vec256){a, b, c, d, a, b, c, d};
#endif
}

ALWAYS_INLINE void vec_shuf0(vec256& x)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    x.lo = __builtin_shufflevector(x.lo, x.lo, 1, 2, 3, 0);
    x.hi = __builtin_shufflevector(x.hi, x.hi, 1, 2, 3, 0);
#else
    x = __builtin_shufflevector(x, x, 1, 2, 3, 0, 5, 6, 7, 4);
#endif
}

ALWAYS_INLINE void vec_shuf1(vec256& x)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    x.lo = __builtin_shufflevector(x.lo, x.lo, 2, 3, 0, 1);
    x.hi = __builtin_shufflevector(x.hi, x.hi, 2, 3, 0, 1);
#else
    x = __builtin_shufflevector(x, x, 2, 3, 0, 1, 6, 7, 4, 5);
#endif
}

ALWAYS_INLINE void vec_shuf2(vec256& x)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    x.lo = __builtin_shufflevector(x.lo, x.lo, 3, 0, 1, 2);
    x.hi = __builtin_shufflevector(x.hi, x.hi, 3, 0, 1, 2);
#else
    x = __builtin_shufflevector(x, x, 3, 0, 1, 2, 7, 4, 5, 6);
#endif
}

ALWAYS_INLINE vec256 vec_pack_hi(const vec256& a, const vec256& b)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    return vec256{a.hi, b.hi};
#else
    return __builtin_shufflevector(a, b, 4, 5, 6, 7, 12, 13, 14, 15);
#endif
}

ALWAYS_INLINE vec256 vec_pack_lo(const vec256& a, const vec256& b)
{
#if CHACHA20_VEC_USE_SPLIT_LANES
    return vec256{a.lo, b.lo};
#else
    return __builtin_shufflevector(a, b, 0, 1, 2, 3, 8, 9, 10, 11);
#endif
}

#if CHACHA20_VEC_USE_SPLIT_LANES
static constexpr vec256 make_increment(uint32_t odd, uint32_t even)
{
    return vec256{{odd, 0, 0, 0}, {even, 0, 0, 0}};
}
#else
static constexpr vec256 make_increment(uint32_t odd, uint32_t even)
{
    return (vec256){odd, 0, 0, 0, even, 0, 0, 0};
}
#endif

// Counter increments for each half-state pair. Pattern: {2*i+1, 0, 0, 0, 2*i, 0, 0, 0}
// All smaller state counts use a prefix of this array.
static constexpr vec256 increments[8] = {
    make_increment(1, 0),
    make_increment(3, 2),
    make_increment(5, 4),
    make_increment(7, 6),
    make_increment(9, 8),
    make_increment(11, 10),
    make_increment(13, 12),
    make_increment(15, 14),
};

/** Store a vector in all array elements */
template <size_t HalfStates>
ALWAYS_INLINE void arr_set_vec256(std::array<vec256, HalfStates>& arr, const vec256& vec)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() { arr[I] = vec; });
}

/** Add a vector to all array elements */
template <size_t HalfStates>
ALWAYS_INLINE void arr_add_vec256(std::array<vec256, HalfStates>& arr, const vec256& vec)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() { arr[I] += vec; });
}

/** Add corresponding vectors in arr1 to arr0 */
template <size_t HalfStates>
ALWAYS_INLINE void arr_add_arr(std::array<vec256, HalfStates>& arr0, const vec256* arr1)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() { arr0[I] += arr1[I]; });
}

/** Add arr1 to arr0, XOR result into arr2, rotate arr2 left by N bits */
template <unsigned N, size_t HalfStates>
ALWAYS_INLINE void arr_add_xor_rot(std::array<vec256, HalfStates>& arr0, const std::array<vec256, HalfStates>& arr1, std::array<vec256, HalfStates>& arr2)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() {
        vec256& x = arr0[I];
        const vec256& y = arr1[I];
        vec256& z = arr2[I];

        x += y;
        z ^= x;
        vec_rotl<N>(z);
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
template <size_t HalfStates>
ALWAYS_INLINE void arr_shuf0(std::array<vec256, HalfStates>& arr)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() { vec_shuf0(arr[I]); });
}

template <size_t HalfStates>
ALWAYS_INLINE void arr_shuf1(std::array<vec256, HalfStates>& arr)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() { vec_shuf1(arr[I]); });
}

template <size_t HalfStates>
ALWAYS_INLINE void arr_shuf2(std::array<vec256, HalfStates>& arr)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() { vec_shuf2(arr[I]); });
}

/* Main round function. */
template <size_t HalfStates>
ALWAYS_INLINE void doubleround(std::array<vec256, HalfStates>& arr0, std::array<vec256, HalfStates>& arr1, std::array<vec256, HalfStates>& arr2, std::array<vec256, HalfStates>& arr3)
{
    UNROLL_LOOP(10)
    for (size_t i = 0; i < 10; ++i) {
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

    vec256 tmp_vec;
    memcpy(&tmp_vec, in_bytes, sizeof(tmp_vec));
    vec_byteswap(tmp_vec);

    tmp_vec ^= vec;
    vec_byteswap(tmp_vec);

    memcpy(out_bytes, &tmp_vec, sizeof(tmp_vec));
}

template <bool AssumeAligned, size_t HalfStates>
ALWAYS_INLINE void arr_read_xor_write_impl(const std::byte* in_bytes, std::byte* out_bytes, const std::array<vec256, HalfStates>& arr0, const std::array<vec256, HalfStates>& arr1, const std::array<vec256, HalfStates>& arr2, const std::array<vec256, HalfStates>& arr3)
{
    util::ForEachIndex<HalfStates>([&]<size_t I>() {
        const vec256& w = arr0[I];
        const vec256& x = arr1[I];
        const vec256& y = arr2[I];
        const vec256& z = arr3[I];

        constexpr size_t offset = I * 128;
        const std::byte* in_slice = in_bytes + offset;
        std::byte* out_slice = out_bytes + offset;

        vec_read_xor_write<AssumeAligned>(in_slice + 0, out_slice + 0, vec_pack_hi(w, x));
        vec_read_xor_write<AssumeAligned>(in_slice + 32, out_slice + 32, vec_pack_hi(y, z));
        vec_read_xor_write<AssumeAligned>(in_slice + 64, out_slice + 64, vec_pack_lo(w, x));
        vec_read_xor_write<AssumeAligned>(in_slice + 96, out_slice + 96, vec_pack_lo(y, z));
    });
}

/* Merge the 128 bit lanes from 2 states to the proper order, then pass each vec_read_xor_write */
template <size_t HalfStates>
ALWAYS_INLINE void arr_read_xor_write(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const std::array<vec256, HalfStates>& arr0, const std::array<vec256, HalfStates>& arr1, const std::array<vec256, HalfStates>& arr2, const std::array<vec256, HalfStates>& arr3)
{
#if defined(__clang__) && (defined(__aarch64__) || defined(__ARM_NEON) || defined(__ARM_NEON__))
    // On AArch64/NEON, clang's codegen for `std::assume_aligned` + 32-byte memcpy
    // can be slower than the unaligned path. Prefer the single unaligned variant.
    arr_read_xor_write_impl<false>(in_bytes.data(), out_bytes.data(), arr0, arr1, arr2, arr3);
#else
    constexpr std::uintptr_t mask{CHACHA20_VEC_MEM_ALIGN - 1};
    const bool aligned = ((reinterpret_cast<std::uintptr_t>(in_bytes.data()) | reinterpret_cast<std::uintptr_t>(out_bytes.data())) & mask) == 0;

    if (aligned) [[likely]] {
        arr_read_xor_write_impl<true>(in_bytes.data(), out_bytes.data(), arr0, arr1, arr2, arr3);
    } else {
        arr_read_xor_write_impl<false>(in_bytes.data(), out_bytes.data(), arr0, arr1, arr2, arr3);
    }
#endif
}

/* Main crypt function. Calculates up to 16 states (8 half_states). */
template <size_t States>
ALWAYS_INLINE void multi_block_crypt(std::span<const std::byte> in_bytes, std::span<std::byte> out_bytes, const vec256& state0, const vec256& state1, const vec256& state2)
{
    static_assert(States == 16 || States == 8 || States == 6 || States == 4 || States == 2);
    constexpr size_t half_states = States / 2;
    std::array<vec256, half_states> arr0, arr1, arr2, arr3;

    arr_set_vec256(arr0, nums256);
    arr_set_vec256(arr1, state0);
    arr_set_vec256(arr2, state1);
    arr_set_vec256(arr3, state2);

    arr_add_arr(arr3, increments);

    doubleround(arr0, arr1, arr2, arr3);

    arr_add_vec256(arr0, nums256);
    arr_add_vec256(arr1, state0);
    arr_add_vec256(arr2, state1);
    arr_add_vec256(arr3, state2);

    arr_add_arr(arr3, increments);

    arr_read_xor_write(in_bytes, out_bytes, arr0, arr1, arr2, arr3);
}

template <size_t States>
ALWAYS_INLINE void process_blocks(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const vec256& state0, const vec256& state1, vec256& state2)
{
    while (in_bytes.size() >= CHACHA20_VEC_BLOCKLEN * States) {
        multi_block_crypt<States>(in_bytes, out_bytes, state0, state1, state2);
        vec_add_counter(state2, static_cast<uint32_t>(States));
        in_bytes = in_bytes.subspan(CHACHA20_VEC_BLOCKLEN * States);
        out_bytes = out_bytes.subspan(CHACHA20_VEC_BLOCKLEN * States);
    }
}

} // anonymous namespace
#endif // CHACHA20_VEC_ENABLE_ANY_MULTI_STATE

#if defined(CHACHA20_NAMESPACE)
namespace CHACHA20_NAMESPACE {
#endif

void chacha20_crypt_vectorized(std::span<const std::byte>& in_bytes, std::span<std::byte>& out_bytes, const std::array<uint32_t, 12>& input) noexcept
{
#if CHACHA20_VEC_ENABLE_ANY_MULTI_STATE
    assert(in_bytes.size() == out_bytes.size());
    const vec256 state0 = vec_broadcast4(input[0], input[1], input[2], input[3]);
    const vec256 state1 = vec_broadcast4(input[4], input[5], input[6], input[7]);
    vec256 state2 = vec_broadcast4(input[8], input[9], input[10], input[11]);

    if constexpr (kEnableStates16) process_blocks<16>(in_bytes, out_bytes, state0, state1, state2);
    if constexpr (kEnableStates8) process_blocks<8>(in_bytes, out_bytes, state0, state1, state2);
    if constexpr (kEnableStates6) process_blocks<6>(in_bytes, out_bytes, state0, state1, state2);
    if constexpr (kEnableStates4) process_blocks<4>(in_bytes, out_bytes, state0, state1, state2);
    if constexpr (kEnableStates2) process_blocks<2>(in_bytes, out_bytes, state0, state1, state2);
#else
    (void)in_bytes;
    (void)out_bytes;
    (void)input;
#endif
}

#if defined(CHACHA20_NAMESPACE)
}
#endif

#undef CHACHA20_VEC_ENABLE_ANY_MULTI_STATE
#undef CHACHA20_VEC_USE_SPLIT_LANES

#endif // ENABLE_CHACHA20_VEC
