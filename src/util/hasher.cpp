// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/hasher.h>

#include <crypto/siphash.h>
#include <random.h>

template <typename Hasher>
static Hasher RandomlySalted()
{
    FastRandomContext rng;
    return Hasher{rng.rand64(), rng.rand64()};
}

SaltedUint256Hasher::SaltedUint256Hasher() : m_hasher{RandomlySalted<PresaltedSipHasher>()} {}

SaltedBlockHashHasher::SaltedBlockHashHasher() : m_hasher{RandomlySalted<SipHasher13UJ>()} {}

SaltedTxidHasher::SaltedTxidHasher() : m_hasher{RandomlySalted<SipHasher13UJ>()} {}

SaltedWtxidHasher::SaltedWtxidHasher() : m_hasher{RandomlySalted<SipHasher13UJ>()} {}

SaltedGenTxidHasher::SaltedGenTxidHasher() : m_hasher{RandomlySalted<SipHasher13UJ>()} {}

SaltedOutpointHasher::SaltedOutpointHasher() : m_hasher{RandomlySalted<PresaltedSipHasher>()} {}

SaltedSipHasher::SaltedSipHasher() :
    m_k0{FastRandomContext().rand64()},
    m_k1{FastRandomContext().rand64()}
{}

size_t SaltedSipHasher::operator()(const std::span<const unsigned char>& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script).Finalize();
}
