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
constexpr uint8_t DB_TXINDEX_HASHED_COUNT{4};
constexpr uint8_t TX_HASH_TAG_BITS{2};
constexpr uint8_t TX_HASH_INLINE_BITS{38};
constexpr uint8_t BLOCK_TX_OFFSET_GRANULARITY{2};
constexpr uint64_t BLOCK_TX_POSITION_FACTOR{2'000'000};
constexpr uint32_t MAX_TXINDEX_BLOCK_HEIGHT{(uint32_t{1} << 21) - 1};
constexpr uint64_t BLOCK_TX_POSITION_LIMIT{BLOCK_TX_POSITION_FACTOR * (uint64_t{MAX_TXINDEX_BLOCK_HEIGHT} + 1)};
constexpr uint8_t BLOCK_TX_POSITION_HIGH_BITS{2};
constexpr uint8_t BLOCK_TX_POSITION_LOW_BITS{40};
constexpr uint64_t BLOCK_TX_POSITION_LOW_MASK{(uint64_t{1} << BLOCK_TX_POSITION_LOW_BITS) - 1};
constexpr uint8_t BLOCK_TX_POSITION_HIGH_MASK{(uint8_t{1} << BLOCK_TX_POSITION_HIGH_BITS) - 1};
static_assert(DB_TXINDEX_HASHED_COUNT == uint8_t{1} << TX_HASH_TAG_BITS);
static_assert(TX_HASH_TAG_BITS + TX_HASH_INLINE_BITS == 40);
static_assert(BLOCK_TX_POSITION_HIGH_BITS + BLOCK_TX_POSITION_LOW_BITS == 42);
static_assert(MAX_BLOCK_SERIALIZED_SIZE / BLOCK_TX_OFFSET_GRANULARITY <= BLOCK_TX_POSITION_FACTOR);
static_assert(MAX_BLOCK_SERIALIZED_SIZE % BLOCK_TX_OFFSET_GRANULARITY == 0);
static_assert(MIN_TRANSACTION_WEIGHT / WITNESS_SCALE_FACTOR >= BLOCK_TX_OFFSET_GRANULARITY);
static_assert(BLOCK_TX_POSITION_LIMIT <= uint64_t{1} << 42);

constexpr bool IsHashedKeyTag(uint8_t tag)
{
    return tag >= DB_TXINDEX_HASHED && tag < DB_TXINDEX_HASHED + DB_TXINDEX_HASHED_COUNT;
}

//! The location of a transaction: the height of the block that contains it and the
//! transaction's serialized byte offset from the start of that block (including the
//! header), rounded down to an adjacent pair. Since valid transactions are at least
//! 60 bytes, each pair contains at most one transaction position. FindTx can try the
//! two positions beginning at block_data_pos + tx_offset_in_block.
//!
//! The packed position is code = 2,000,000 * height + offset / 2. Heights through
//! 2,097,151 and every valid block offset fit in 42 bits.
struct BlockTxPosition {
    uint32_t block_height{0};
    uint32_t tx_offset_in_block{0};

    BlockTxPosition() = default;
    BlockTxPosition(uint32_t height, uint32_t tx_offset)
        : block_height{height}, tx_offset_in_block{tx_offset - tx_offset % BLOCK_TX_OFFSET_GRANULARITY}
    {
    }

    friend bool operator==(const BlockTxPosition&, const BlockTxPosition&) = default;

    bool ContainsOffset(uint32_t tx_offset) const
    {
        return tx_offset >= tx_offset_in_block && tx_offset - tx_offset_in_block < BLOCK_TX_OFFSET_GRANULARITY;
    }

    uint64_t GetCode() const
    {
        if (block_height > MAX_TXINDEX_BLOCK_HEIGHT) throw std::ios_base::failure("Block height exceeds txindex position format");
        assert(tx_offset_in_block < MAX_BLOCK_SERIALIZED_SIZE);
        assert(tx_offset_in_block % BLOCK_TX_OFFSET_GRANULARITY == 0);
        return BLOCK_TX_POSITION_FACTOR * block_height + tx_offset_in_block / BLOCK_TX_OFFSET_GRANULARITY;
    }

    void SetCode(uint64_t code)
    {
        if (code >= BLOCK_TX_POSITION_LIMIT) throw std::ios_base::failure("Invalid txindex transaction position");
        block_height = static_cast<uint32_t>(code / BLOCK_TX_POSITION_FACTOR);
        tx_offset_in_block = static_cast<uint32_t>(code % BLOCK_TX_POSITION_FACTOR) * BLOCK_TX_OFFSET_GRANULARITY;
    }
};

struct TxHashKeyPrefix {
    static constexpr int SERIALIZED_SIZE{6};

    uint8_t db_tag{DB_TXINDEX_HASHED};
    std::array<std::byte, 5> hash{};

    friend bool operator==(const TxHashKeyPrefix&, const TxHashKeyPrefix&) = default;

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        assert(IsHashedKeyTag(db_tag));
        assert((std::to_integer<uint8_t>(hash.back()) & BLOCK_TX_POSITION_HIGH_MASK) == 0);
        ser_writedata8(s, db_tag);
        s << hash;
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        db_tag = ser_readdata8(s);
        s >> hash;
        if (!IsHashedKeyTag(db_tag) || (std::to_integer<uint8_t>(hash.back()) & BLOCK_TX_POSITION_HIGH_MASK) != 0) {
            throw std::ios_base::failure("Invalid format for txindex hash prefix");
        }
    }
};

inline TxHashKeyPrefix CreateKeyPrefix(const PresaltedSipHasher& hasher, const Txid& txid)
{
    const uint64_t hash{hasher(txid.ToUint256())};
    const uint64_t inline_hash{((hash >> (64 - TX_HASH_TAG_BITS - TX_HASH_INLINE_BITS)) & ((uint64_t{1} << TX_HASH_INLINE_BITS) - 1)) << BLOCK_TX_POSITION_HIGH_BITS};
    std::array<std::byte, sizeof(uint64_t)> be_hash;
    WriteBE64(be_hash.data(), inline_hash);
    TxHashKeyPrefix prefix{static_cast<uint8_t>(DB_TXINDEX_HASHED + (hash >> (64 - TX_HASH_TAG_BITS)))};
    std::memcpy(prefix.hash.data(), be_hash.data() + be_hash.size() - prefix.hash.size(), prefix.hash.size());
    return prefix;
}

struct DBKey {
    static constexpr int SERIALIZED_SIZE{11};

    TxHashKeyPrefix hash_prefix;
    BlockTxPosition pos;

    explicit DBKey(const TxHashKeyPrefix& hash_in, const BlockTxPosition& pos_in) : hash_prefix{hash_in}, pos{pos_in} {}

    template <typename Stream>
    void Serialize(Stream& s) const
    {
        assert(IsHashedKeyTag(hash_prefix.db_tag));
        assert((std::to_integer<uint8_t>(hash_prefix.hash.back()) & BLOCK_TX_POSITION_HIGH_MASK) == 0);
        const uint64_t position{pos.GetCode()};
        std::array<std::byte, 5> hash{hash_prefix.hash};
        hash.back() |= std::byte{static_cast<uint8_t>(position >> BLOCK_TX_POSITION_LOW_BITS)};
        ser_writedata8(s, hash_prefix.db_tag);
        s << hash << Using<BigEndianFormatter<5>>(position & BLOCK_TX_POSITION_LOW_MASK);
    }

    template <typename Stream>
    void Unserialize(Stream& s)
    {
        hash_prefix.db_tag = ser_readdata8(s);
        s >> hash_prefix.hash;
        if (!IsHashedKeyTag(hash_prefix.db_tag)) throw std::ios_base::failure("Invalid format for txindex DB key");
        const uint8_t position_high{static_cast<uint8_t>(std::to_integer<uint8_t>(hash_prefix.hash.back()) & BLOCK_TX_POSITION_HIGH_MASK)};
        hash_prefix.hash.back() &= std::byte{static_cast<uint8_t>(~BLOCK_TX_POSITION_HIGH_MASK)};
        uint64_t position_low;
        s >> Using<BigEndianFormatter<5>>(position_low);
        pos.SetCode(uint64_t{position_high} << BLOCK_TX_POSITION_LOW_BITS | position_low);
        if (!s.empty()) throw std::ios_base::failure("Invalid size for txindex DB key");
    }
};
} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
