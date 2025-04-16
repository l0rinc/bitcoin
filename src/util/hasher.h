// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_HASHER_H
#define BITCOIN_UTIL_HASHER_H

#include <crypto/common.h>
#include <crypto/siphash.h>
#include <primitives/transaction.h>
#include <span.h>
#include <uint256.h>

#include <cstdint>
#include <cstring>

class SaltedTxidHasher
{
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedTxidHasher();

    size_t operator()(const uint256& txid) const {
        return SipHashUint256(k0, k1, txid);
    }
};

class SaltedOutpointHasher
{
private:
    /** Salt */
    const uint64_t k0, k1;

public:
    SaltedOutpointHasher(bool deterministic = false);

    size_t operator()(const COutPoint& id) const {
        return SipHashUint256Extra(k0, k1, id.hash, id.n);
    }
};

struct FilterHeaderHasher
{
    size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
};

/**
 * We're hashing a nonce into the entries themselves, so we don't need extra
 * blinding in the set hash computation.
 *
 * This may exhibit platform endian dependent behavior but because these are
 * nonced hashes (random) and this state is only ever used locally it is safe.
 * All that matters is local consistency.
 */
class SignatureCacheHasher
{
public:
    template <uint8_t hash_select>
    uint32_t operator()(const uint256& key) const
    {
        static_assert(hash_select <8, "SignatureCacheHasher only has 8 hashes available.");
        uint32_t u;
        std::memcpy(&u, key.begin()+4*hash_select, 4);
        return u;
    }
};

struct BlockHasher
{
    // this used to call `GetCheapHash()` in uint256, which was later moved; the
    // cheap hash function simply calls ReadLE64() however, so the end result is
    // identical
    size_t operator()(const uint256& hash) const { return ReadLE64(hash.begin()); }
};

class SaltedSipHasher
{
private:
    /** Salt */
    const uint64_t m_k0, m_k1;

public:
    SaltedSipHasher();

    size_t operator()(const std::span<const unsigned char>& script) const;
};

#endif // BITCOIN_UTIL_HASHER_H
