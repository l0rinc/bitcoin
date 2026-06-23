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

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ios>
#include <string>

namespace txindex {
constexpr uint8_t DB_TXINDEX_HASHED{'x'};

//! The location of a transaction: the height of the block that contains it and the
//! transaction's serialized byte offset from the start of that block (including the
//! header), so the on-disk position is simply block_data_pos + tx_offset_in_block.
//!
//! Since the offset is always less than the maximum serialized block size, we pack
//! the position into a single integer code = max_block_size * height + offset, and
//! split it apart as (height = code / max_block_size, offset = code % max_block_size).
struct BlockTxPosition {
    uint32_t block_height{0};
    uint32_t tx_offset_in_block{0};

    friend bool operator==(const BlockTxPosition&, const BlockTxPosition&) = default;

    static constexpr int SERIALIZED_SIZE{6}; // Holds packed positions until height 70,368,744

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        assert(tx_offset_in_block < MAX_BLOCK_SERIALIZED_SIZE);
        const uint64_t code{uint64_t{MAX_BLOCK_SERIALIZED_SIZE} * block_height + tx_offset_in_block};
        s << Using<BigEndianFormatter<SERIALIZED_SIZE>>(code);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        uint64_t code;
        s >> Using<BigEndianFormatter<SERIALIZED_SIZE>>(code);
        block_height = static_cast<uint32_t>(code / MAX_BLOCK_SERIALIZED_SIZE);
        tx_offset_in_block = static_cast<uint32_t>(code % MAX_BLOCK_SERIALIZED_SIZE);
    }
};

using TxHashKeyPrefix = std::array<std::byte, 5>;

inline TxHashKeyPrefix CreateKeyPrefix(const PresaltedSipHasher& hasher, const Txid& txid)
{
    // Normalize to big-endian so prefixes are stable across architectures.
    std::array<std::byte, sizeof(uint64_t)> be_hash;
    WriteBE64(be_hash.data(), hasher(txid.ToUint256()));
    TxHashKeyPrefix prefix;
    std::memcpy(prefix.data(), be_hash.data(), prefix.size());
    return prefix;
}

struct DBKey {
    TxHashKeyPrefix hash_prefix;
    BlockTxPosition pos;

    explicit DBKey(const TxHashKeyPrefix& hash_in, const BlockTxPosition& pos_in) : hash_prefix{hash_in}, pos{pos_in} {}

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        ser_writedata8(s, DB_TXINDEX_HASHED);
        s << hash_prefix << pos;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        if (ser_readdata8(s) != DB_TXINDEX_HASHED) throw std::ios_base::failure("Invalid format for txindex DB key");
        s >> hash_prefix >> pos;
    }
};
} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
