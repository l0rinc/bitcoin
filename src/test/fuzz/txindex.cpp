// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <consensus/consensus.h>
#include <crypto/siphash.h>
#include <index/txindex_key.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <vector>

FUZZ_TARGET(txindex_position)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const uint8_t shuffle_attempts{provider.ConsumeIntegralInRange<uint8_t>(0, 7)};
    FastRandomContext rng{ConsumeUInt256(provider)};
    const PresaltedSipHasher hasher{rng.rand64(), rng.rand64()};
    constexpr uint32_t min_tx_size{MIN_TRANSACTION_WEIGHT / WITNESS_SCALE_FACTOR};
    constexpr uint32_t max_tx_count{MAX_BLOCK_WEIGHT / MIN_TRANSACTION_WEIGHT};

    std::vector<uint32_t> tx_sizes;
    uint32_t remaining{MAX_BLOCK_SERIALIZED_SIZE - static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) - 9};
    while (remaining >= min_tx_size && tx_sizes.size() < max_tx_count && provider.remaining_bytes() > 0) {
        const uint32_t tx_size{provider.ConsumeIntegralInRange<uint32_t>(min_tx_size, remaining)};
        tx_sizes.push_back(tx_size);
        remaining -= tx_size;
    }
    if (tx_sizes.empty()) tx_sizes.push_back(min_tx_size);

    for (uint8_t attempt{0}; attempt <= shuffle_attempts; ++attempt) {
        const uint32_t height{rng.randrange(txindex::MAX_TXINDEX_BLOCK_HEIGHT + 1)};
        const auto prefix{txindex::CreateKeyPrefix(hasher, Txid::FromUint256(rng.rand256()))};
        std::vector<uint32_t> tx_offsets;
        uint32_t tx_offset{static_cast<uint32_t>(GetSerializeSize(CBlockHeader{})) + GetSizeOfCompactSize(tx_sizes.size())};
        for (const uint32_t tx_size : tx_sizes) {
            tx_offsets.push_back(tx_offset);
            tx_offset += tx_size;
        }
        assert(tx_offset <= MAX_BLOCK_SERIALIZED_SIZE);

        DataStream prefix_stream;
        prefix_stream << prefix;
        assert(prefix_stream.size() == txindex::TxHashKeyPrefix::SERIALIZED_SIZE);

        for (const uint32_t expected_offset : tx_offsets) {
            DataStream stream;
            stream << txindex::DBKey{prefix, txindex::BlockTxPosition{height, expected_offset}};
            assert(stream.size() == txindex::DBKey::SERIALIZED_SIZE);

            txindex::DBKey decoded{prefix, {}};
            stream >> decoded;
            assert(stream.empty());
            assert(decoded.hash_prefix == prefix);
            assert(decoded.pos.block_height == height);

            const auto match{std::lower_bound(tx_offsets.begin(), tx_offsets.end(), decoded.pos.tx_offset_in_block)};
            assert(match != tx_offsets.end());
            assert(decoded.pos.ContainsOffset(*match));
            assert(*match == expected_offset);
            assert(std::next(match) == tx_offsets.end() || !decoded.pos.ContainsOffset(*std::next(match)));
        }
        if (attempt < shuffle_attempts) std::shuffle(tx_sizes.begin(), tx_sizes.end(), rng);
    }
}
