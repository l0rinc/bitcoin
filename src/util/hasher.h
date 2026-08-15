// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_HASHER_H
#define BITCOIN_UTIL_HASHER_H

#include <crypto/common.h>
#include <crypto/siphash.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <cstdint>
#include <cstring>
#include <span>

// The salted SipHash-1-3 hashers below differ in how they compress a 256-bit key: as four normal
// blocks, which assumes nothing about the key, or as a single jumbo block, which saves 3 rounds
// but requires the key to be the output of a cryptographic hash. Pick by the provenance of the key
// rather than by its type: a hash a peer sent us is untrusted even though it has the same type
// as one we computed ourselves.
//
// All of them are salted per process, so their outputs are process-local and must not be
// persisted, serialized, or compared across processes.

/**
 * Hasher for containers that may retain keys of unknown provenance.
 *
 * Compresses its input as normal blocks, so it makes no assumption about the key at the cost of
 * 3 more rounds than the jumbo hashers. Use it whenever a container can retain a hash a peer sent
 * us rather than one we computed ourselves.
 */
class SaltedUntrustedHasher
{
    const SipHasher13UJ m_hasher;

public:
    SaltedUntrustedHasher();

    size_t operator()(const uint256& hash) const noexcept
    {
        return m_hasher.HashNormal(hash);
    }

    size_t operator()(const Txid& txid) const noexcept
    {
        return m_hasher.HashNormal(txid.ToUint256());
    }
};

/**
 * Hasher for containers keyed by block hashes we computed ourselves.
 *
 * Every key is the double-SHA256 output of a block header, so it may be compressed as a single
 * jumbo block even when the block has not passed full validation. Use SaltedUntrustedHasher for
 * uint256 values received from a peer without hashing them first.
 */
class SaltedBlockHashHasher
{
    const SipHasher13UJ m_hasher;

public:
    SaltedBlockHashHasher();

    size_t operator()(const uint256& hash) const
    {
        return m_hasher.HashJumbo(hash);
    }
};

/**
 * Hashers for containers keyed by transaction identifiers we computed ourselves.
 *
 * Every container using them keys on the identifier of a transaction it holds, so the keys are
 * always cryptographic hash outputs and may be compressed as a single jumbo block. Containers
 * that can retain an identifier a peer chose, such as the prevouts of a transaction we have not
 * validated, must use SaltedUntrustedHasher instead.
 */
class SaltedTxidHasher
{
    const SipHasher13UJ m_hasher;

public:
    SaltedTxidHasher();

    size_t operator()(const Txid& txid) const
    {
        return m_hasher.HashJumbo(txid.ToUint256());
    }
};

class SaltedWtxidHasher
{
    const SipHasher13UJ m_hasher;

public:
    SaltedWtxidHasher();

    size_t operator()(const Wtxid& wtxid) const
    {
        return m_hasher.HashJumbo(wtxid.ToUint256());
    }
};

/** Hasher for generic transaction identifiers we computed ourselves. */
class SaltedGenTxidHasher
{
    const SipHasher13UJ m_hasher;

public:
    SaltedGenTxidHasher();

    size_t operator()(const GenTxid& gtxid) const
    {
        // The txid/wtxid discriminator is part of GenTxid equality, so include it as a normal block.
        return m_hasher.HashJumbo(gtxid.ToUint256(), uint64_t{gtxid.IsWtxid()});
    }
};

/**
 * Hasher for outpoint keyed containers.
 *
 * Outpoints reach these containers as claimed prevouts of transactions we have not validated yet,
 * so unlike SaltedCoinsCacheHasher this cannot assume the txid is the output of a cryptographic
 * hash, and compresses it as four normal blocks instead of one jumbo block.
 */
class SaltedOutpointHasher
{
    const SipHasher13UJ m_hasher;

public:
    SaltedOutpointHasher();

    /**
     * Having the hash noexcept allows libstdc++'s unordered_map to recalculate
     * the hash during rehash, so it does not have to cache the value. This
     * reduces node's memory by sizeof(size_t). The required recalculation has
     * a slight performance penalty (around 1.6%), but this is compensated by
     * memory savings of about 9% which allow for a larger dbcache setting.
     *
     * @see https://gcc.gnu.org/onlinedocs/gcc-13.2.0/libstdc++/manual/manual/unordered_associative.html
     */
    size_t operator()(const COutPoint& id) const noexcept
    {
        return m_hasher.HashNormal(id.hash.ToUint256(), uint64_t{id.n});
    }
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
