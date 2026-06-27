// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <merkleblock.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(merkleblock)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CPartialMerkleTree partial_merkle_tree;
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            const std::optional<CPartialMerkleTree> opt_partial_merkle_tree = ConsumeDeserializable<CPartialMerkleTree>(fuzzed_data_provider);
            if (opt_partial_merkle_tree) {
                partial_merkle_tree = *opt_partial_merkle_tree;
            }
        },
        [&] {
            CMerkleBlock merkle_block;
            const std::optional<CBlock> opt_block = ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS);
            CBloomFilter bloom_filter;
            std::set<Txid> txids;
            if (opt_block && !opt_block->vtx.empty()) {
                if (fuzzed_data_provider.ConsumeBool()) {
                    merkle_block = CMerkleBlock{*opt_block, bloom_filter};
                } else if (fuzzed_data_provider.ConsumeBool()) {
                    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
                        txids.insert(Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)));
                    }
                    merkle_block = CMerkleBlock{*opt_block, txids};
                }
            }
            partial_merkle_tree = merkle_block.txn;
        });
    (void)partial_merkle_tree.GetNumTransactions();
    std::vector<Txid> matches{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};
    std::vector<unsigned int> indices{std::numeric_limits<unsigned int>::max()};
    const uint256 merkle_root{partial_merkle_tree.ExtractMatches(matches, indices)};
    assert(matches.size() == indices.size());
    for (size_t i{0}; i < indices.size(); ++i) {
        assert(indices[i] < partial_merkle_tree.GetNumTransactions());
        if (i > 0) {
            assert(indices[i] > indices[i - 1]);
        }
    }
    (void)merkle_root;
}
