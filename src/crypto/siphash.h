// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_SIPHASH_H
#define BITCOIN_CRYPTO_SIPHASH_H

#include <cstdint>
#include <span>

class uint256;

class SipSalt
{
    static constexpr uint64_t C0{0x736f6d6570736575ULL}, C1{0x646f72616e646f6dULL}, C2{0x6c7967656e657261ULL}, C3{0x7465646279746573ULL};

public:
    explicit SipSalt(uint64_t k0, uint64_t k1) noexcept : v{C0 ^ k0, C1 ^ k1, C2 ^ k0, C3 ^ k1} {}

    std::array<uint64_t, 4> v;
};

/** SipHash-2-4 */
class CSipHasher
{
    SipSalt m_salt;
    uint64_t m_tmp;
    uint8_t m_count; // Only the low 8 bits of the input size matter.

public:
    /** Construct a SipHash calculator initialized with 128-bit key (k0, k1) */
    CSipHasher(uint64_t k0, uint64_t k1);
    /** Hash a 64-bit integer worth of data
     *  It is treated as if this was the little-endian interpretation of 8 bytes.
     *  This function can only be used when a multiple of 8 bytes have been written so far.
     */
    CSipHasher& Write(uint64_t data);
    /** Hash arbitrary bytes. */
    CSipHasher& Write(std::span<const unsigned char> data);
    /** Compute the 64-bit SipHash-2-4 of the data written so far. The object remains untouched. */
    uint64_t Finalize() const;
};

// Optimized SipHash-2-4 implementation for uint256.
class PresaltedSipHasher
{
    const SipSalt m_salt;

public:
    explicit PresaltedSipHasher(uint64_t k0, uint64_t k1) noexcept : m_salt{k0, k1} {}

    uint64_t operator()(const uint256& val) const noexcept;
    uint64_t operator()(const uint256& val, uint32_t extra) const noexcept;
};

#endif // BITCOIN_CRYPTO_SIPHASH_H
