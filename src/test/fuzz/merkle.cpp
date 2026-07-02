// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/str.h>
#include <util/strencodings.h>
#include <hash.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace {

uint256 ComputeMerkleRootReference(std::vector<uint256> hashes, bool* mutated = nullptr)
{
    bool mutation{false};
    while (hashes.size() > 1) {
        std::vector<uint256> next_level;
        next_level.reserve((hashes.size() + 1) / 2);
        for (size_t pos{0}; pos < hashes.size(); pos += 2) {
            const uint256& left{hashes[pos]};
            const uint256& right{pos + 1 < hashes.size() ? hashes[pos + 1] : hashes[pos]};
            if (pos + 1 < hashes.size() && left == right) mutation = true;
            next_level.push_back(Hash(left, right));
        }
        hashes = std::move(next_level);
    }
    if (mutated) *mutated = mutation;
    return hashes.empty() ? uint256{} : hashes.front();
}

size_t ExpectedMerklePathSize(size_t leaves)
{
    if (leaves <= 1) return 0;
    size_t depth{0};
    --leaves;
    while (leaves > 0) {
        ++depth;
        leaves >>= 1;
    }
    return depth;
}

uint256 ComputeMerkleRootFromPath(const CBlock& block, uint32_t position, const std::vector<uint256>& merkle_path)
{
    if (position >= block.vtx.size()) {
        throw std::out_of_range("Position out of range");
    }

    uint256 current_hash = block.vtx[position]->GetHash().ToUint256();

    for (const uint256& sibling : merkle_path) {
        if (position % 2 == 0) {
            current_hash = Hash(current_hash, sibling);
        } else {
            current_hash = Hash(sibling, current_hash);
        }
        position = position / 2;
    }

    return current_hash;
}

} // namespace

FUZZ_TARGET(merkle)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const bool with_witness = fuzzed_data_provider.ConsumeBool();
    std::optional<CBlock> block{ConsumeDeserializable<CBlock>(fuzzed_data_provider, with_witness ? TX_WITH_WITNESS : TX_NO_WITNESS)};
    if (!block) {
        return;
    }
    const size_t num_txs = block->vtx.size();
    for (const auto& tx : block->vtx) {
        assert(tx != nullptr);
    }
    std::vector<uint256> tx_hashes;
    tx_hashes.reserve(num_txs);

    for (size_t i = 0; i < num_txs; ++i) {
        tx_hashes.push_back(block->vtx[i]->GetHash().ToUint256());
    }

    bool reference_mutated{fuzzed_data_provider.ConsumeBool()}; // output sentinel must be overwritten.
    const uint256 reference_merkle_root{ComputeMerkleRootReference(tx_hashes, &reference_mutated)};

    bool optimized_mutated{!reference_mutated}; // output sentinel must be overwritten.
    const uint256 merkle_root{ComputeMerkleRoot(tx_hashes, &optimized_mutated)};
    assert(merkle_root == reference_merkle_root);
    assert(optimized_mutated == reference_mutated);
    assert(ComputeMerkleRoot(tx_hashes) == merkle_root);

    if (tx_hashes.empty()) {
        assert(merkle_root.IsNull());
        assert(!optimized_mutated);
    } else if (tx_hashes.size() == 1) {
        assert(merkle_root == tx_hashes[0]);
        assert(!optimized_mutated);
    }

    bool block_mutated{!reference_mutated}; // output sentinel must be overwritten.
    const uint256 block_merkle_root{BlockMerkleRoot(*block, &block_mutated)};
    assert(block_merkle_root == merkle_root);
    assert(block_mutated == reference_mutated);
    assert(BlockMerkleRoot(*block) == merkle_root);

    std::vector<uint256> witness_hashes;
    witness_hashes.reserve(num_txs > 0 ? num_txs : 1);
    witness_hashes.emplace_back(); // The witness hash of the coinbase is 0.
    for (size_t i{1}; i < num_txs; ++i) {
        witness_hashes.push_back(block->vtx[i]->GetWitnessHash().ToUint256());
    }
    const uint256 block_witness_merkle_root{BlockWitnessMerkleRoot(*block)};
    assert(block_witness_merkle_root == ComputeMerkleRootReference(std::move(witness_hashes)));
    if (tx_hashes.size() == 1) assert(block_witness_merkle_root == uint256());

    // Test TransactionMerklePath
    const uint32_t position = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, num_txs > 0 ? num_txs - 1 : 0);
    std::vector<uint256> merkle_path = TransactionMerklePath(*block, position);
    assert(merkle_path.size() == ExpectedMerklePathSize(num_txs));
    if (num_txs > 0) {
        const uint256 merkle_root_from_merkle_path{ComputeMerkleRootFromPath(*block, position, merkle_path)};
        assert(merkle_root_from_merkle_path == merkle_root);
        assert(merkle_root_from_merkle_path == block_merkle_root);
    }

    assert(merkle_path.size() <= 32); // Maximum depth of a Merkle tree with 2^32 leaves
    if (num_txs == 1 || num_txs == 0) assert(merkle_path.empty());
}
