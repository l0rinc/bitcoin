// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20_vec.h>

#include <attributes.h>
#include <compat/byteswap.h>

#include <array>
#include <bit>
#include <cassert>
#include <cstring>

#if defined(ENABLE_CHACHA20_VEC)

#if defined(CHACHA20_VEC_DISABLE_STATES_16) && \
    defined(CHACHA20_VEC_DISABLE_STATES_8) && \
    defined(CHACHA20_VEC_DISABLE_STATES_6) && \
    defined(CHACHA20_VEC_DISABLE_STATES_4) && \
    defined(CHACHA20_VEC_DISABLE_STATES_2)
#  define CHACHA20_VEC_ALL_STATES_DISABLED
#endif

#if !defined(CHACHA20_VEC_ALL_STATES_DISABLED)

namespace {

using Vec256 = uint32_t __attribute__((__vector_size__(32)));
static_assert(sizeof(Vec256) == 32);

// These helpers recurse over compile-time indices to make the intended
// unrolling explicit in this performance-sensitive code.
ALWAYS_INLINE void VecByteswap(Vec256& v) noexcept
{
    if constexpr (std::endian::native == std::endian::big) {
        for (size_t i{0}; i < 8; ++i) {
            v[i] = internal_bswap_32(v[i]);
        }
    }
}

template <size_t BITS>
ALWAYS_INLINE void VecRotl(Vec256& v) noexcept
{
    v = (v << BITS) | (v >> (32 - BITS));
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void Fill(std::array<Vec256, N>& arr, const Vec256& v) noexcept
{
    std::get<I>(arr) = v;
    if constexpr (I + 1 < N) Fill<N, I + 1>(arr, v);
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void Add(std::array<Vec256, N>& arr, const Vec256& v) noexcept
{
    std::get<I>(arr) += v;
    if constexpr (I + 1 < N) Add<N, I + 1>(arr, v);
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void Add(std::array<Vec256, N>& dst, const std::array<Vec256, N>& src) noexcept
{
    std::get<I>(dst) += std::get<I>(src);
    if constexpr (I + 1 < N) Add<N, I + 1>(dst, src);
}

template <size_t BITS, size_t N, size_t I = 0>
ALWAYS_INLINE void AddXorRot(std::array<Vec256, N>& a, const std::array<Vec256, N>& b, std::array<Vec256, N>& d) noexcept
{
    std::get<I>(a) += std::get<I>(b);
    std::get<I>(d) ^= std::get<I>(a);
    VecRotl<BITS>(std::get<I>(d));
    if constexpr (I + 1 < N) AddXorRot<BITS, N, I + 1>(a, b, d);
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void Shuffle0(std::array<Vec256, N>& arr) noexcept
{
    Vec256& x{std::get<I>(arr)};
    x = Vec256{x[1], x[2], x[3], x[0], x[5], x[6], x[7], x[4]};
    if constexpr (I + 1 < N) Shuffle0<N, I + 1>(arr);
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void Shuffle1(std::array<Vec256, N>& arr) noexcept
{
    Vec256& x{std::get<I>(arr)};
    x = Vec256{x[2], x[3], x[0], x[1], x[6], x[7], x[4], x[5]};
    if constexpr (I + 1 < N) Shuffle1<N, I + 1>(arr);
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void Shuffle2(std::array<Vec256, N>& arr) noexcept
{
    Vec256& x{std::get<I>(arr)};
    x = Vec256{x[3], x[0], x[1], x[2], x[7], x[4], x[5], x[6]};
    if constexpr (I + 1 < N) Shuffle2<N, I + 1>(arr);
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void DoubleRounds(std::array<Vec256, N>& a, std::array<Vec256, N>& b, std::array<Vec256, N>& c, std::array<Vec256, N>& d) noexcept
{
    AddXorRot<16>(a, b, d);
    AddXorRot<12>(c, d, b);
    AddXorRot<8>(a, b, d);
    AddXorRot<7>(c, d, b);
    Shuffle0(b);
    Shuffle1(c);
    Shuffle2(d);
    AddXorRot<16>(a, b, d);
    AddXorRot<12>(c, d, b);
    AddXorRot<8>(a, b, d);
    AddXorRot<7>(c, d, b);
    Shuffle2(b);
    Shuffle1(c);
    Shuffle0(d);
    if constexpr (I + 1 < 10) DoubleRounds<N, I + 1>(a, b, c, d);
}

ALWAYS_INLINE void WriteXor(std::span<const std::byte, 32> input, std::span<std::byte, 32> output, const Vec256& stream_in) noexcept
{
    Vec256 stream{stream_in};
    VecByteswap(stream);

    std::array<uint32_t, 8> words;
    std::memcpy(words.data(), input.data(), input.size());
    stream ^= Vec256{words[0], words[1], words[2], words[3], words[4], words[5], words[6], words[7]};
    words = {stream[0], stream[1], stream[2], stream[3], stream[4], stream[5], stream[6], stream[7]};
    std::memcpy(output.data(), words.data(), output.size());
}

template <size_t N, size_t I = 0>
ALWAYS_INLINE void WriteBlocks(std::span<const std::byte> input, std::span<std::byte> output, const std::array<Vec256, N>& a, const std::array<Vec256, N>& b, const std::array<Vec256, N>& c, const std::array<Vec256, N>& d) noexcept
{
    const Vec256& w{std::get<I>(a)};
    const Vec256& x{std::get<I>(b)};
    const Vec256& y{std::get<I>(c)};
    const Vec256& z{std::get<I>(d)};

    WriteXor(input.first<32>(), output.first<32>(), __builtin_shufflevector(w, x, 4, 5, 6, 7, 12, 13, 14, 15));
    WriteXor(input.subspan<32, 32>(), output.subspan<32, 32>(), __builtin_shufflevector(y, z, 4, 5, 6, 7, 12, 13, 14, 15));
    WriteXor(input.subspan<64, 32>(), output.subspan<64, 32>(), __builtin_shufflevector(w, x, 0, 1, 2, 3, 8, 9, 10, 11));
    WriteXor(input.subspan<96, 32>(), output.subspan<96, 32>(), __builtin_shufflevector(y, z, 0, 1, 2, 3, 8, 9, 10, 11));

    if constexpr (I + 1 < N) WriteBlocks<N, I + 1>(input.subspan<128>(), output.subspan<128>(), a, b, c, d);
}

template <size_t N>
consteval std::array<Vec256, N> MakeIncrements()
{
    std::array<Vec256, N> ret;
    for (uint32_t i{0}; i < N; ++i) {
        ret[i] = (i * Vec256{2, 0, 0, 0, 2, 0, 0, 0}) + Vec256{1, 0, 0, 0, 0, 0, 0, 0};
    }
    return ret;
}

template <size_t STATES>
ALWAYS_INLINE void CryptBlocks(std::span<const std::byte> input, std::span<std::byte> output, const Vec256& key0, const Vec256& key1, const Vec256& counter_nonce) noexcept
{
    static_assert(STATES % 2 == 0);
    static constexpr size_t HALF_STATES{STATES / 2};
    static constexpr Vec256 constants{0x61707865, 0x3320646e, 0x79622d32, 0x6b206574, 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    static constinit const std::array<Vec256, HALF_STATES> increments{MakeIncrements<HALF_STATES>()};

    std::array<Vec256, HALF_STATES> a;
    std::array<Vec256, HALF_STATES> b;
    std::array<Vec256, HALF_STATES> c;
    std::array<Vec256, HALF_STATES> d;

    Fill(a, constants);
    Fill(b, key0);
    Fill(c, key1);
    Fill(d, counter_nonce);
    Add(d, increments);

    DoubleRounds(a, b, c, d);

    Add(a, constants);
    Add(b, key0);
    Add(c, key1);
    Add(d, counter_nonce);
    Add(d, increments);

    WriteBlocks(input, output, a, b, c, d);
}

template <size_t STATES>
ALWAYS_INLINE void ProcessChunk(std::span<const std::byte>& input, std::span<std::byte>& output, const Vec256& key0, const Vec256& key1, Vec256& counter_nonce) noexcept
{
    while (input.size() >= CHACHA20_VEC_BLOCKLEN * STATES) {
        CryptBlocks<STATES>(input, output, key0, key1, counter_nonce);
        counter_nonce += Vec256{STATES, 0, 0, 0, STATES, 0, 0, 0};
        input = input.subspan(CHACHA20_VEC_BLOCKLEN * STATES);
        output = output.subspan(CHACHA20_VEC_BLOCKLEN * STATES);
    }
}

} // namespace

#endif // !defined(CHACHA20_VEC_ALL_STATES_DISABLED)

#if defined(CHACHA20_VEC_NAMESPACE)
namespace CHACHA20_VEC_NAMESPACE {
#endif

void Crypt(std::span<const std::byte>& input, std::span<std::byte>& output, const std::array<uint32_t, 12>& state) noexcept
{
#if !defined(CHACHA20_VEC_ALL_STATES_DISABLED)
    assert(input.size() == output.size());
    assert(input.size() % CHACHA20_VEC_BLOCKLEN == 0);

    const Vec256 key0{state[0], state[1], state[2], state[3], state[0], state[1], state[2], state[3]};
    const Vec256 key1{state[4], state[5], state[6], state[7], state[4], state[5], state[6], state[7]};
    Vec256 counter_nonce{state[8], state[9], state[10], state[11], state[8], state[9], state[10], state[11]};

#if !defined(CHACHA20_VEC_DISABLE_STATES_16)
    ProcessChunk<16>(input, output, key0, key1, counter_nonce);
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_8)
    ProcessChunk<8>(input, output, key0, key1, counter_nonce);
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_6)
    ProcessChunk<6>(input, output, key0, key1, counter_nonce);
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_4)
    ProcessChunk<4>(input, output, key0, key1, counter_nonce);
#endif
#if !defined(CHACHA20_VEC_DISABLE_STATES_2)
    ProcessChunk<2>(input, output, key0, key1, counter_nonce);
#endif
#endif // !defined(CHACHA20_VEC_ALL_STATES_DISABLED)
}

#if defined(CHACHA20_VEC_NAMESPACE)
} // namespace CHACHA20_VEC_NAMESPACE
#endif

#endif // defined(ENABLE_CHACHA20_VEC)
