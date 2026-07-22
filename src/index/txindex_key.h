// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_TXINDEX_KEY_H
#define BITCOIN_INDEX_TXINDEX_KEY_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>

#include <cstdint>
#include <ios>
#include <string>
#include <utility>

namespace txindex {
constexpr uint8_t DB_TXINDEX{'t'};
constexpr uint8_t DB_BLOCK_SEQ{'s'};
constexpr uint8_t DB_BLOCK_HASH{'h'};
inline constexpr std::string DB_NEXT_BLOCK_SEQ{"next_block_seq"};
constexpr int BLOCK_SEQ_SIZE{3};

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

inline std::pair<uint8_t, uint256> LegacyTxKey(const Txid& txid)
{
    return {DB_TXINDEX, txid.ToUint256()};
}
} // namespace txindex

#endif // BITCOIN_INDEX_TXINDEX_KEY_H
