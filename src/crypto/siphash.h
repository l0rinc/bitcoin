// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SIPHASH_H
#define BITCOIN_CRYPTO_SIPHASH_H

#include <attributes.h>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <uint256.h>

namespace siphash_detail {

ALWAYS_INLINE void SipRound(uint64_t& v0, uint64_t& v1, uint64_t& v2, uint64_t& v3)
{
    // Copy in/out to avoid potential pessimizations from aliasing concerns.
    uint64_t a{v0}, b{v1}, c{v2}, d{v3};

    a += b; b = std::rotl(b, 13); b ^= a;
    a = std::rotl(a, 32);
    c += d; d = std::rotl(d, 16); d ^= c;
    a += d; d = std::rotl(d, 21); d ^= a;
    c += b; b = std::rotl(b, 17); b ^= c;
    c = std::rotl(c, 32);

    v0 = a;
    v1 = b;
    v2 = c;
    v3 = d;
}

} // namespace siphash_detail

/** Shared SipHash internal state v[0..3], initialized from (k0, k1). */
class SipHashState
{
    static constexpr uint64_t C0{0x736f6d6570736575ULL}, C1{0x646f72616e646f6dULL}, C2{0x6c7967656e657261ULL}, C3{0x7465646279746573ULL};

public:
    explicit SipHashState(uint64_t k0, uint64_t k1) noexcept : v{C0 ^ k0, C1 ^ k1, C2 ^ k0, C3 ^ k1} {}

    std::array<uint64_t, 4> v{};
};

/** General SipHash-2-4 implementation. */
class CSipHasher
{
    SipHashState m_state;
    uint64_t m_tmp{0};
    uint8_t m_count{0}; //!< Only the low 8 bits of the input size matter.

public:
    /** Construct a SipHash calculator initialized with 128-bit key (k0, k1). */
    CSipHasher(uint64_t k0, uint64_t k1);
    /** Hash a 64-bit integer worth of data.
     *  It is treated as if this was the little-endian interpretation of 8 bytes.
     *  This function can only be used when a multiple of 8 bytes have been written so far.
     */
    CSipHasher& Write(uint64_t data);
    /** Hash arbitrary bytes. */
    CSipHasher& Write(std::span<const unsigned char> data);
    /** Compute the 64-bit SipHash-2-4 of the data written so far. The object remains untouched. */
    uint64_t Finalize() const;
};

/** General SipHash-1-3 implementation. */
class CSipHasher13
{
    SipHashState m_state;
    uint64_t m_tmp{0};
    uint8_t m_count{0}; //!< Only the low 8 bits of the input size matter.

public:
    /** Construct a SipHash calculator initialized with 128-bit key (k0, k1). */
    CSipHasher13(uint64_t k0, uint64_t k1);
    /** Hash a 64-bit integer worth of data.
     *  It is treated as if this was the little-endian interpretation of 8 bytes.
     *  This function can only be used when a multiple of 8 bytes have been written so far.
     */
    CSipHasher13& Write(uint64_t data);
    /** Hash arbitrary bytes. */
    CSipHasher13& Write(std::span<const unsigned char> data);
    /** Compute the 64-bit SipHash-1-3 of the data written so far. The object remains untouched. */
    uint64_t Finalize() const;
};

/**
 * Optimized SipHash-2-4 implementation for uint256.
 *
 * This class caches the initial SipHash v[0..3] state derived from (k0, k1)
 * and implements a specialized hashing path for uint256 values, with or
 * without an extra 32-bit word. The internal state is immutable, so
 * PresaltedSipHasher24 instances can be reused for multiple hashes with the
 * same key.
 */
class PresaltedSipHasher24
{
    const SipHashState m_state;

public:
    explicit PresaltedSipHasher24(uint64_t k0, uint64_t k1) noexcept : m_state{k0, k1} {}

    /** Equivalent to CSipHasher(k0, k1).Write(val).Finalize(). */
    uint64_t operator()(const uint256& val) const noexcept;

    /**
     * Equivalent to CSipHasher(k0, k1).Write(val).Write(extra).Finalize(),
     * with `extra` encoded as 4 little-endian bytes.
     */
    uint64_t operator()(const uint256& val, uint32_t extra) const noexcept;
};

/**
 * Optimized SipHash‑1‑3 implementation for uint256.
 *
 * This class caches the initial SipHash v[0..3] state derived from (k0, k1)
 * and implements a specialized hashing path for uint256 values, with or
 * without an extra 32-bit word. The internal state is immutable, so
 * PresaltedSipHasher13 instances can be reused for multiple hashes with the
 * same key.
 */
class PresaltedSipHasher13
{
    const SipHashState m_state;

public:
    explicit PresaltedSipHasher13(uint64_t k0, uint64_t k1) noexcept : m_state{k0, k1} {}

    /** Equivalent to CSipHasher13(k0, k1).Write(val).Finalize(). */
    uint64_t operator()(const uint256& val) const noexcept;

    /**
     * Equivalent to CSipHasher13(k0, k1).Write(val).Write(extra).Finalize(),
     * with `extra` encoded as 4 little-endian bytes.
     */
    uint64_t operator()(const uint256& val, uint32_t extra) const noexcept;
};

/**
 * Optimized SipHash‑1‑3 implementation for 256‑bit inputs using "jumboblock"
 * processing.
 *
 * This is a nonstandard variant intended for internal hash-table use when the
 * input is already uniformly distributed (i.e. a hash). It injects the 4 limbs
 * of a uint256 in parallel, reducing the number of compression rounds needed
 * for 32-byte inputs.
 */
class PresaltedSipHasher13Jumbo
{
    const SipHashState m_state;

public:
    explicit PresaltedSipHasher13Jumbo(uint64_t k0, uint64_t k1) noexcept : m_state{k0, k1} {}

    uint64_t operator()(const uint256& val) const noexcept;
    uint64_t operator()(const uint256& val, uint32_t extra) const noexcept;
};

inline uint64_t PresaltedSipHasher24::operator()(const uint256& val) const noexcept
{
    using siphash_detail::SipRound;
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;

    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    v3 ^= (uint64_t{4}) << 59;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= (uint64_t{4}) << 59;
    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

inline uint64_t PresaltedSipHasher24::operator()(const uint256& val, uint32_t extra) const noexcept
{
    using siphash_detail::SipRound;
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = ((uint64_t{36}) << 56) | extra;
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

inline uint64_t PresaltedSipHasher13::operator()(const uint256& val) const noexcept
{
    using siphash_detail::SipRound;
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;

    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    v3 ^= (uint64_t{4}) << 59;
    SipRound(v0, v1, v2, v3);
    v0 ^= (uint64_t{4}) << 59;
    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

inline uint64_t PresaltedSipHasher13::operator()(const uint256& val, uint32_t extra) const noexcept
{
    using siphash_detail::SipRound;
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    d = ((uint64_t{36}) << 56) | extra;
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;
    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

inline uint64_t PresaltedSipHasher13Jumbo::operator()(const uint256& val) const noexcept
{
    using siphash_detail::SipRound;
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    const uint64_t m0{val.GetUint64(0)}, m1{val.GetUint64(1)}, m2{val.GetUint64(2)}, m3{val.GetUint64(3)};

    v0 ^= m0;
    v1 ^= m1;
    v2 ^= m2;
    v3 ^= m3;
    SipRound(v0, v1, v2, v3);
    v0 ^= m3;
    v1 ^= m0;
    v2 ^= m1;
    v3 ^= m2;

    uint64_t d{(uint64_t{4}) << 59};
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;

    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

inline uint64_t PresaltedSipHasher13Jumbo::operator()(const uint256& val, uint32_t extra) const noexcept
{
    using siphash_detail::SipRound;
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    const uint64_t m0{val.GetUint64(0)};
    const uint64_t m1{val.GetUint64(1)};
    const uint64_t m2{val.GetUint64(2)};
    const uint64_t m3{val.GetUint64(3)};

    v0 ^= m0;
    v1 ^= m1;
    v2 ^= m2;
    v3 ^= m3;
    SipRound(v0, v1, v2, v3);
    v0 ^= m3;
    v1 ^= m0;
    v2 ^= m1;
    v3 ^= m2;

    uint64_t d{((uint64_t{36}) << 56) | extra};
    v3 ^= d;
    SipRound(v0, v1, v2, v3);
    v0 ^= d;

    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

#endif // BITCOIN_CRYPTO_SIPHASH_H
