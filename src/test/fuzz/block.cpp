// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <hash.h>
#include <pubkey.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>
#include <validation.h>

#include <algorithm>
#include <cassert>
#include <string>

namespace {

void AssertBlockFieldsEqual(const CBlock& block, const CBlock& round_tripped)
{
    assert(block.nVersion == round_tripped.nVersion);
    assert(block.hashPrevBlock == round_tripped.hashPrevBlock);
    assert(block.hashMerkleRoot == round_tripped.hashMerkleRoot);
    assert(block.nTime == round_tripped.nTime);
    assert(block.nBits == round_tripped.nBits);
    assert(block.nNonce == round_tripped.nNonce);
    assert(block.vtx.size() == round_tripped.vtx.size());
    for (size_t i{0}; i < block.vtx.size(); ++i) {
        assert(*block.vtx[i] == *round_tripped.vtx[i]);
    }
}

} // namespace

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
    const Consensus::Params& consensus_params = Params().GetConsensus();
    BlockValidationState validation_state_pow_and_merkle;
    const bool valid_incl_pow_and_merkle = CheckBlock(block, validation_state_pow_and_merkle, consensus_params, /* fCheckPOW= */ true, /* fCheckMerkleRoot= */ true);
    assert(validation_state_pow_and_merkle.IsValid() || validation_state_pow_and_merkle.IsInvalid() || validation_state_pow_and_merkle.IsError());
    (void)validation_state_pow_and_merkle.Error("");
    BlockValidationState validation_state_pow;
    const bool valid_incl_pow = CheckBlock(block, validation_state_pow, consensus_params, /* fCheckPOW= */ true, /* fCheckMerkleRoot= */ false);
    assert(validation_state_pow.IsValid() || validation_state_pow.IsInvalid() || validation_state_pow.IsError());
    BlockValidationState validation_state_merkle;
    const bool valid_incl_merkle = CheckBlock(block, validation_state_merkle, consensus_params, /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ true);
    assert(validation_state_merkle.IsValid() || validation_state_merkle.IsInvalid() || validation_state_merkle.IsError());
    BlockValidationState validation_state_none;
    const bool valid_incl_none = CheckBlock(block, validation_state_none, consensus_params, /* fCheckPOW= */ false, /* fCheckMerkleRoot= */ false);
    assert(validation_state_none.IsValid() || validation_state_none.IsInvalid() || validation_state_none.IsError());
    const bool has_multiple_coinbases = std::count_if(block.vtx.begin(), block.vtx.end(), [](const auto& tx) {
                                            return tx->IsCoinBase();
                                        }) > 1;
    const bool has_invalid_coinbase_layout = block.vtx.empty() ||
                                             !block.vtx.front()->IsCoinBase() || has_multiple_coinbases;
    if (has_invalid_coinbase_layout) {
        // These context-free consensus rules must hold even when PoW and merkle checks are disabled.
        assert(!valid_incl_none);
    }
    if (valid_incl_pow_and_merkle) {
        assert(valid_incl_pow && valid_incl_merkle && valid_incl_none);
    } else if (valid_incl_merkle || valid_incl_pow) {
        assert(valid_incl_none);
    }
    DataStream serialized_with_witness;
    serialized_with_witness << TX_WITH_WITNESS(block);
    DataStream serialized_without_witness;
    serialized_without_witness << TX_NO_WITNESS(block);
    assert(serialized_with_witness.size() == ::GetSerializeSize(TX_WITH_WITNESS(block)));
    assert(serialized_without_witness.size() == ::GetSerializeSize(TX_NO_WITNESS(block)));

    DataStream round_trip_stream;
    round_trip_stream << TX_WITH_WITNESS(block);
    CBlock round_tripped;
    round_trip_stream >> TX_WITH_WITNESS(round_tripped);
    assert(round_trip_stream.empty());
    AssertBlockFieldsEqual(block, round_tripped);

    DataStream reserialized_round_trip;
    reserialized_round_trip << TX_WITH_WITNESS(round_tripped);
    assert(serialized_with_witness.size() == reserialized_round_trip.size());
    assert(std::equal(serialized_with_witness.begin(), serialized_with_witness.end(), reserialized_round_trip.begin(), reserialized_round_trip.end()));

    const uint256 expected_hash{(HashWriter{} << static_cast<const CBlockHeader&>(block)).GetHash()};
    assert(block.GetHash() == expected_hash);
    assert(round_tripped.GetHash() == expected_hash);
    const int64_t expected_weight{
        static_cast<int64_t>(serialized_without_witness.size()) * (WITNESS_SCALE_FACTOR - 1) +
        static_cast<int64_t>(serialized_with_witness.size())};
    assert(GetBlockWeight(block) == expected_weight);
    if (valid_incl_merkle) {
        bool mutated{false};
        assert(block.hashMerkleRoot == BlockMerkleRoot(block, &mutated));
        assert(!mutated);
    }
    (void)block.ToString();
    if (!block.vtx.empty()) {
        (void)BlockWitnessMerkleRoot(block);
    }
    (void)GetWitnessCommitmentIndex(block);
    const size_t raw_memory_size = RecursiveDynamicUsage(block);
    const size_t raw_memory_size_as_shared_ptr = RecursiveDynamicUsage(std::make_shared<CBlock>(block));
    assert(raw_memory_size_as_shared_ptr > raw_memory_size);
    CBlock block_copy = block;
    block_copy.SetNull();
    const bool is_null = block_copy.IsNull();
    assert(is_null);
    assert(block_copy.vtx.empty());
    assert(!block_copy.fChecked);
    assert(!block_copy.m_checked_witness_commitment);
    assert(!block_copy.m_checked_merkle_root);
}
