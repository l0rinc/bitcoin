// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/siphash.h>
#include <random.h>
#include <span.h>
#include <util/hasher.h>

constexpr uint64_t DETERMINISTIC_K0{0x8e819f2607a18de6}, DETERMINISTIC_K1{0xf4020d2e3983b0eb};

SaltedUint256Hasher::SaltedUint256Hasher() : m_hasher{
    FastRandomContext().rand64(),
    FastRandomContext().rand64()}
{}

SaltedTxidHasher::SaltedTxidHasher() : m_hasher{
    FastRandomContext().rand64(),
    FastRandomContext().rand64()}
{}

SaltedWtxidHasher::SaltedWtxidHasher() : m_hasher{
    FastRandomContext().rand64(),
    FastRandomContext().rand64()}
{}

SaltedOutpointHasher24::SaltedOutpointHasher24(bool deterministic) : m_hasher{
    deterministic ? DETERMINISTIC_K0 : FastRandomContext().rand64(),
    deterministic ? DETERMINISTIC_K1 : FastRandomContext().rand64()}
{}

SaltedOutpointHasher13::SaltedOutpointHasher13(bool deterministic) : m_hasher{
    deterministic ? DETERMINISTIC_K0 : FastRandomContext().rand64(),
    deterministic ? DETERMINISTIC_K1 : FastRandomContext().rand64()}
{}

SaltedOutpointHasher13Jumbo::SaltedOutpointHasher13Jumbo(bool deterministic) : m_hasher{
    deterministic ? DETERMINISTIC_K0 : FastRandomContext().rand64(),
    deterministic ? DETERMINISTIC_K1 : FastRandomContext().rand64()}
{}

SaltedSipHasher::SaltedSipHasher() :
    m_k0{FastRandomContext().rand64()},
    m_k1{FastRandomContext().rand64()}
{}

size_t SaltedSipHasher::operator()(const std::span<const unsigned char>& script) const
{
    return CSipHasher(m_k0, m_k1).Write(script).Finalize();
}
