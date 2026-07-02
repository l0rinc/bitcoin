// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <primitives/block.h>
#include <pubkey.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <validation.h>

#include <cassert>
#include <string>
#include <utility>
#include <vector>

void initialize_block()
{
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(block, .init = initialize_block)
{
    CBlock block;
    try {
        SpanReader{buffer} >> TX_WITH_WITNESS(block);
    } catch (const std::ios_base::failure&) {
        return;
    }
    for (const auto& tx : block.vtx) {
        Assert(tx != nullptr);
    }
    const Consensus::Params& consensus_params = Params().GetConsensus();
    const uint256 block_hash{block.GetHash()};
    const uint256 merkle_root{block.hashMerkleRoot};
    std::vector<Txid> txids;
    txids.reserve(block.vtx.size());
    for (const auto& tx : block.vtx) {
        txids.push_back(tx->GetHash());
    }

    bool merkle_mutated{false};
    const bool valid_merkle_root{
        BlockMerkleRoot(block, &merkle_mutated) == block.hashMerkleRoot && !merkle_mutated};

    auto assert_block_unchanged = [&](const CBlock& checked_block) {
        assert(checked_block.GetHash() == block_hash);
        assert(checked_block.hashMerkleRoot == merkle_root);
        assert(checked_block.vtx.size() == txids.size());
        for (size_t i{0}; i < checked_block.vtx.size(); ++i) {
            assert(checked_block.vtx[i]->GetHash() == txids[i]);
        }
    };
    auto check_block = [&](const bool check_pow, const bool check_merkle_root) {
        CBlock checked_block{block};
        BlockValidationState validation_state;
        const bool valid{
            CheckBlock(checked_block, validation_state, consensus_params, check_pow, check_merkle_root)};
        assert(validation_state.IsValid() || validation_state.IsInvalid() || validation_state.IsError());
        assert_block_unchanged(checked_block);
        if (check_pow && check_merkle_root) {
            assert(checked_block.fChecked == valid);
        } else {
            assert(!checked_block.fChecked);
        }
        if (!check_pow && check_merkle_root) {
            assert(checked_block.m_checked_merkle_root == valid_merkle_root);
        }
        return std::pair{valid, checked_block};
    };

    auto pow_and_merkle_check{check_block(/*check_pow=*/true, /*check_merkle_root=*/true)};
    const bool valid_incl_pow_and_merkle{pow_and_merkle_check.first};
    const bool valid_incl_pow{check_block(/*check_pow=*/true, /*check_merkle_root=*/false).first};
    const bool valid_incl_merkle{check_block(/*check_pow=*/false, /*check_merkle_root=*/true).first};
    const bool valid_incl_none{check_block(/*check_pow=*/false, /*check_merkle_root=*/false).first};
    if (valid_incl_pow_and_merkle) {
        assert(valid_incl_pow && valid_incl_merkle && valid_incl_none);
        BlockValidationState cached_validation_state;
        assert(CheckBlock(pow_and_merkle_check.second, cached_validation_state, consensus_params,
            /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ false));
        assert(cached_validation_state.IsValid());
        assert_block_unchanged(pow_and_merkle_check.second);
    } else if (valid_incl_merkle || valid_incl_pow) {
        assert(valid_incl_none);
    }
    (void)block.GetHash();
    (void)block.ToString();
    (void)BlockMerkleRoot(block);
    if (!block.vtx.empty()) {
        (void)BlockWitnessMerkleRoot(block);
    }
    (void)GetBlockWeight(block);
    (void)GetWitnessCommitmentIndex(block);
    const size_t raw_memory_size = RecursiveDynamicUsage(block);
    const size_t raw_memory_size_as_shared_ptr = RecursiveDynamicUsage(std::make_shared<CBlock>(block));
    assert(raw_memory_size_as_shared_ptr > raw_memory_size);
    CBlock block_copy = block;
    block_copy.SetNull();
    const bool is_null = block_copy.IsNull();
    assert(is_null);
}
