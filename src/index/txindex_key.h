// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_KEY_H
#define BITCOIN_INDEX_TXINDEX_KEY_H

#include <consensus/consensus.h>
#include <crypto/common.h>
#include <crypto/siphash.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ios>
#include <string>
#include <utility>

namespace txindex {
constexpr uint8_t DB_TXINDEX{'t'};
constexpr uint8_t DB_TXINDEX_HASHED{'x'};
constexpr uint8_t DB_BLOCK_SEQ{'s'};
constexpr uint8_t DB_BLOCK_HASH{'h'};
inline constexpr std::string DB_TXID_HASH_SALT{"txid_hash_salt"};
inline constexpr std::string DB_NEXT_BLOCK_SEQ{"next_block_seq"};
constexpr int BLOCK_SEQ_SIZE{3};
constexpr int TX_OFFSET_SIZE{3};

//! The location of a transaction: the sequence number of the block that contains it
//! and the transaction's serialized byte offset from the start of that block
//! (including the header), so the on-disk position is simply
//! block_data_pos + tx_offset_in_block.
//!
//! Both values are serialized as 3-byte big-endian integers. The block sequence
//! is complemented so entries sort by descending sequence, then ascending offset.
//! Each value must be below 16,777,216.
struct BlockTxPosition {
    uint32_t block_seq{0};
    uint32_t tx_offset_in_block{0};

    friend bool operator==(const BlockTxPosition&, const BlockTxPosition&) = default;

    static constexpr uint32_t BLOCK_SEQ_MASK{BigEndianFormatter<BLOCK_SEQ_SIZE>::MAX};
    static_assert(MAX_BLOCK_SERIALIZED_SIZE <= BigEndianFormatter<TX_OFFSET_SIZE>::MAX);

    SERIALIZE_METHODS(BlockTxPosition, obj)
    {
        uint32_t ordered_block_seq{obj.block_seq ^ BLOCK_SEQ_MASK};
        READWRITE(Using<BigEndianFormatter<BLOCK_SEQ_SIZE>>(ordered_block_seq),
                  Using<BigEndianFormatter<TX_OFFSET_SIZE>>(obj.tx_offset_in_block));
        SER_READ(obj, obj.block_seq = ordered_block_seq ^ BLOCK_SEQ_MASK);
    }
};

//! Key for looking up the hash of the block with the given sequence number.
struct BlockSeqKey {
    uint32_t block_seq{0};

    SERIALIZE_METHODS(BlockSeqKey, obj)
    {
        uint8_t prefix{DB_BLOCK_SEQ};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_BLOCK_SEQ) throw std::ios_base::failure("Invalid format for txindex block seq key");
        READWRITE(Using<BigEndianFormatter<BLOCK_SEQ_SIZE>>(obj.block_seq));
    }
};

//! Key for looking up the sequence number assigned to the block with the given hash.
struct BlockHashKey {
    uint256 block_hash;

    SERIALIZE_METHODS(BlockHashKey, obj)
    {
        uint8_t prefix{DB_BLOCK_HASH};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_BLOCK_HASH) throw std::ios_base::failure("Invalid format for txindex block hash key");
        READWRITE(obj.block_hash);
    }
};

//! Key of a legacy (pre-hashing) txindex row: the full txid under the 't' prefix.
inline std::pair<uint8_t, uint256> LegacyTxKey(const Txid& txid)
{
    return {DB_TXINDEX, txid.ToUint256()};
}

using TxHashKeyPrefix = std::array<std::byte, 5>;

inline TxHashKeyPrefix CreateKeyPrefix(const SipHasher13UJ& hasher, const Txid& txid)
{
    std::array<std::byte, sizeof(uint64_t)> be_hash;
    WriteBE64(be_hash.data(), hasher.Hash(txid.ToUint256()));
    TxHashKeyPrefix prefix;
    std::memcpy(prefix.data(), be_hash.data(), prefix.size());
    return prefix;
}

struct DBKey {
    TxHashKeyPrefix hash_prefix;
    BlockTxPosition pos;

    SERIALIZE_METHODS(DBKey, obj)
    {
        uint8_t prefix{DB_TXINDEX_HASHED};
        READWRITE(prefix);
        if (ser_action.ForRead() && prefix != DB_TXINDEX_HASHED) throw std::ios_base::failure("Invalid format for txindex DB key");
        READWRITE(obj.hash_prefix, obj.pos);
    }
};
} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
