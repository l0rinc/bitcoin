// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/common.h>
#include <crypto/siphash.h>

#include <uint256.h>

#include <algorithm>
#include <bit>
#include <cassert>
#include <span>

#define SIPROUND do { \
    v0 += v1; v1 = std::rotl(v1, 13); v1 ^= v0; \
    v0 = std::rotl(v0, 32); \
    v2 += v3; v3 = std::rotl(v3, 16); v3 ^= v2; \
    v0 += v3; v3 = std::rotl(v3, 21); v3 ^= v0; \
    v2 += v1; v1 = std::rotl(v1, 17); v1 ^= v2; \
    v2 = std::rotl(v2, 32); \
} while (0)

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1) : m_state{k0, k1} {}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    assert(m_count % 8 == 0);

    v3 ^= data;
    SIPROUND;
    SIPROUND;
    v0 ^= data;

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;

    m_count += 8;
    return *this;
}

/** Load a little-endian uint64 from 0 to 7 bytes. */
static uint64_t ReadU64ByLenLE(const unsigned char* data, size_t len)
{
    assert(len < 8);
    uint64_t out{0};
    for (size_t i{0}; i < len; ++i) {
        out |= uint64_t{data[i]} << (i * 8);
    }
    return out;
}

CSipHasher& CSipHasher::Write(std::span<const unsigned char> data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    const size_t ntail{static_cast<size_t>(m_count & 0x07)};
    m_count = static_cast<uint8_t>(m_count + data.size());

    size_t needed{0};
    if (ntail != 0) {
        needed = 8 - ntail;
        m_tmp |= ReadU64ByLenLE(data.data(), std::min(data.size(), needed)) << (8 * ntail);
        if (data.size() < needed) {
            return *this;
        } else {
            v3 ^= m_tmp;
            SIPROUND;
            SIPROUND;
            v0 ^= m_tmp;
        }
    }

    const size_t len{data.size() - needed};
    const size_t left{len & 0x07};

    size_t i{needed};
    while (i < len - left) {
        const uint64_t mi{ReadLE64(data.data() + i)};
        v3 ^= mi;
        SIPROUND;
        SIPROUND;
        v0 ^= mi;
        i += 8;
    }

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;
    m_tmp = left ? ReadU64ByLenLE(data.data() + i, left) : 0;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    uint64_t t = m_tmp | (((uint64_t)m_count) << 56);

    v3 ^= t;
    SIPROUND;
    SIPROUND;
    v0 ^= t;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

uint64_t PresaltedSipHasher::operator()(const uint256& val) const noexcept
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;

    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v3 ^= (uint64_t{4}) << 59;
    SIPROUND;
    SIPROUND;
    v0 ^= (uint64_t{4}) << 59;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

/** Specialized implementation for efficiency */
uint64_t PresaltedSipHasher::operator()(const uint256& val, uint32_t extra) const noexcept
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t d = val.GetUint64(0);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = val.GetUint64(3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = ((uint64_t{36}) << 56) | extra;
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}
