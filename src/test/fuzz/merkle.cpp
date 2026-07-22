// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>

#include <hash.h>
#include <primitives/block.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace {

struct MerkleResult {
    uint256 root;
    bool mutated;
};

MerkleResult ReferenceComputeMerkleRoot(std::vector<uint256> hashes)
{
    bool mutated{false};
    while (hashes.size() > 1) {
        for (size_t position{0}; position + 1 < hashes.size(); position += 2) {
            if (hashes[position] == hashes[position + 1]) {
                mutated = true;
            }
        }

        std::vector<uint256> next_level;
        next_level.reserve(hashes.size() / 2 + hashes.size() % 2);
        for (size_t position{0}; position < hashes.size(); position += 2) {
            const size_t sibling{position + 1 < hashes.size() ? position + 1 : position};
            next_level.push_back(Hash(hashes[position], hashes[sibling]));
        }
        hashes = std::move(next_level);
    }
    return {hashes.empty() ? uint256{} : hashes.front(), mutated};
}

std::vector<uint256> ReferenceMerklePath(const std::vector<uint256>& leaves, uint32_t position)
{
    if (leaves.empty()) return {};
    assert(position < leaves.size());

    std::vector<uint256> level{leaves};
    std::vector<uint256> path;
    size_t index{position};
    while (level.size() > 1) {
        const size_t sibling{(index ^ 1) < level.size() ? index ^ 1 : index};
        path.push_back(level[sibling]);

        std::vector<uint256> next_level;
        next_level.reserve(level.size() / 2 + level.size() % 2);
        for (size_t offset{0}; offset < level.size(); offset += 2) {
            const size_t right{offset + 1 < level.size() ? offset + 1 : offset};
            next_level.push_back(Hash(level[offset], level[right]));
        }
        level = std::move(next_level);
        index >>= 1;
    }
    return path;
}

uint256 ReferenceMerkleRootFromPath(uint256 leaf, const std::vector<uint256>& path, uint32_t position)
{
    for (const uint256& sibling : path) {
        leaf = (position & 1) ? Hash(sibling, leaf) : Hash(leaf, sibling);
        position >>= 1;
    }
    return leaf;
}

std::vector<uint256> TransactionHashes(const CBlock& block)
{
    std::vector<uint256> hashes;
    hashes.reserve(block.vtx.size());
    for (const CTransactionRef& transaction : block.vtx) {
        hashes.push_back(transaction->GetHash().ToUint256());
    }
    return hashes;
}

MerkleResult ReferenceBlockWitnessMerkleRoot(const CBlock& block)
{
    std::vector<uint256> hashes;
    hashes.reserve((block.vtx.size() + 1) & ~1ULL);
    hashes.emplace_back();
    for (size_t position{1}; position < block.vtx.size(); ++position) {
        hashes.push_back(block.vtx[position]->GetWitnessHash().ToUint256());
    }
    return ReferenceComputeMerkleRoot(std::move(hashes));
}

std::vector<std::byte> SerializeBlock(const CBlock& block)
{
    DataStream stream;
    stream << TX_WITH_WITNESS(block);
    return {stream.begin(), stream.end()};
}

struct BlockSnapshot {
    std::vector<std::byte> serialized;
    bool checked;
    bool checked_witness_commitment;
    bool checked_merkle_root;
};

BlockSnapshot Snapshot(const CBlock& block)
{
    return {SerializeBlock(block), block.fChecked, block.m_checked_witness_commitment, block.m_checked_merkle_root};
}

void AssertUnchanged(const CBlock& block, const BlockSnapshot& before)
{
    assert(SerializeBlock(block) == before.serialized);
    assert(block.fChecked == before.checked);
    assert(block.m_checked_witness_commitment == before.checked_witness_commitment);
    assert(block.m_checked_merkle_root == before.checked_merkle_root);
}

} // namespace

FUZZ_TARGET(merkle)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const bool with_witness{fuzzed_data_provider.ConsumeBool()};
    const std::optional<CBlock> block{ConsumeDeserializable<CBlock>(
        fuzzed_data_provider, with_witness ? TX_WITH_WITNESS : TX_NO_WITNESS)};
    if (!block) return;

    const BlockSnapshot before{Snapshot(*block)};
    const std::vector<uint256> transaction_hashes{TransactionHashes(*block)};
    const MerkleResult expected{ReferenceComputeMerkleRoot(transaction_hashes)};

    bool actual_mutated{fuzzed_data_provider.ConsumeBool()};
    const uint256 actual_root{ComputeMerkleRoot(transaction_hashes, &actual_mutated)};
    assert(actual_root == expected.root);
    assert(actual_mutated == expected.mutated);
    assert(ComputeMerkleRoot(transaction_hashes) == expected.root);
    AssertUnchanged(*block, before);

    bool actual_block_mutated{fuzzed_data_provider.ConsumeBool()};
    const uint256 actual_block_root{BlockMerkleRoot(*block, &actual_block_mutated)};
    assert(actual_block_root == expected.root);
    assert(actual_block_mutated == expected.mutated);
    assert(BlockMerkleRoot(*block) == expected.root);
    AssertUnchanged(*block, before);

    const MerkleResult expected_witness{ReferenceBlockWitnessMerkleRoot(*block)};
    const uint256 actual_witness_root{BlockWitnessMerkleRoot(*block)};
    assert(actual_witness_root == expected_witness.root);
    assert(BlockWitnessMerkleRoot(*block) == expected_witness.root);
    AssertUnchanged(*block, before);

    const uint32_t position{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(
        0, transaction_hashes.empty() ? 0 : transaction_hashes.size() - 1)};
    const std::vector<uint256> expected_path{ReferenceMerklePath(transaction_hashes, position)};
    const std::vector<uint256> actual_path{TransactionMerklePath(*block, position)};
    assert(actual_path == expected_path);
    assert(actual_path.size() <= 32);
    if (!transaction_hashes.empty()) {
        assert(ReferenceMerkleRootFromPath(transaction_hashes[position], actual_path, position) == expected.root);
    }
    AssertUnchanged(*block, before);
}
