// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>

#include <bit>
#include <cassert>
#include <span>

using siphash_detail::SipRound;

CSipHasher::CSipHasher(uint64_t k0, uint64_t k1) : m_state{k0, k1} {}

CSipHasher& CSipHasher::Write(uint64_t data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    assert(m_count % 8 == 0);

    v3 ^= data;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= data;

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;

    m_count += 8;
    return *this;
}

CSipHasher& CSipHasher::Write(std::span<const unsigned char> data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t t = m_tmp;
    uint8_t c = m_count;

    while (data.size() > 0) {
        t |= uint64_t{data.front()} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SipRound(v0, v1, v2, v3);
            SipRound(v0, v1, v2, v3);
            v0 ^= t;
            t = 0;
        }
        data = data.subspan(1);
    }

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;
    m_count = c;
    m_tmp = t;

    return *this;
}

uint64_t CSipHasher::Finalize() const
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    uint64_t t = m_tmp | (((uint64_t)m_count) << 56);

    v3 ^= t;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    v0 ^= t;
    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

CSipHasher13::CSipHasher13(uint64_t k0, uint64_t k1) : m_state{k0, k1} {}

CSipHasher13& CSipHasher13::Write(uint64_t data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    assert(m_count % 8 == 0);

    v3 ^= data;
    SipRound(v0, v1, v2, v3);
    v0 ^= data;

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;

    m_count += 8;
    return *this;
}

CSipHasher13& CSipHasher13::Write(std::span<const unsigned char> data)
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];
    uint64_t t = m_tmp;
    uint8_t c = m_count;

    while (data.size() > 0) {
        t |= uint64_t{data.front()} << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SipRound(v0, v1, v2, v3);
            v0 ^= t;
            t = 0;
        }
        data = data.subspan(1);
    }

    m_state.v[0] = v0;
    m_state.v[1] = v1;
    m_state.v[2] = v2;
    m_state.v[3] = v3;
    m_count = c;
    m_tmp = t;

    return *this;
}

uint64_t CSipHasher13::Finalize() const
{
    uint64_t v0 = m_state.v[0], v1 = m_state.v[1], v2 = m_state.v[2], v3 = m_state.v[3];

    uint64_t t = m_tmp | (((uint64_t)m_count) << 56);

    v3 ^= t;
    SipRound(v0, v1, v2, v3);
    v0 ^= t;
    v2 ^= 0xFF;
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    SipRound(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}
