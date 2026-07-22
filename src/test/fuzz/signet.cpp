// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <signet.h>

#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <script/interpreter.h>
#include <script/verify_flags.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <vector>

namespace {

int ReferenceWitnessCommitmentIndex(const CBlock& block)
{
    static constexpr std::array<uint8_t, 6> commitment_prefix{OP_RETURN, 0x24, 0xaa, 0x21, 0xa9, 0xed};
    int commitment_index{NO_WITNESS_COMMITMENT};
    if (block.vtx.empty()) return commitment_index;

    for (size_t output_index{0}; output_index < block.vtx[0]->vout.size(); ++output_index) {
        const CScript& script{block.vtx[0]->vout[output_index].scriptPubKey};
        if (script.size() >= MINIMUM_WITNESS_COMMITMENT && std::equal(commitment_prefix.begin(), commitment_prefix.end(), script.begin())) {
            commitment_index = output_index;
        }
    }
    return commitment_index;
}

bool ReferenceFetchAndClearCommitmentSection(CScript& witness_commitment, std::vector<uint8_t>& result)
{
    static constexpr std::array<uint8_t, 4> signet_header{0xec, 0xc7, 0xda, 0xa2};
    CScript replacement;
    bool found_header{false};
    result.clear();

    opcodetype opcode;
    CScript::const_iterator pc{witness_commitment.begin()};
    std::vector<uint8_t> pushdata;
    while (witness_commitment.GetOp(pc, opcode, pushdata)) {
        if (!pushdata.empty()) {
            if (!found_header && pushdata.size() > signet_header.size() &&
                std::equal(signet_header.begin(), signet_header.end(), pushdata.begin())) {
                result.insert(result.end(), pushdata.begin() + signet_header.size(), pushdata.end());
                pushdata.resize(signet_header.size());
                found_header = true;
            }
            replacement << pushdata;
        } else {
            replacement << opcode;
        }
    }

    if (found_header) witness_commitment = replacement;
    return found_header;
}

uint256 ReferenceModifiedMerkleRoot(const CMutableTransaction& coinbase, const CBlock& block)
{
    std::vector<uint256> leaves;
    leaves.reserve((block.vtx.size() + 1) & ~1ULL);
    leaves.push_back(coinbase.GetHash().ToUint256());
    for (size_t transaction_index{1}; transaction_index < block.vtx.size(); ++transaction_index) {
        leaves.push_back(block.vtx[transaction_index]->GetHash().ToUint256());
    }
    return ComputeMerkleRoot(std::move(leaves));
}

struct ReferenceSignetTxs {
    CTransaction to_spend;
    CTransaction to_sign;
};

std::optional<ReferenceSignetTxs> ReferenceCreate(const CBlock& block, const CScript& challenge)
{
    CMutableTransaction tx_to_spend;
    tx_to_spend.version = 0;
    tx_to_spend.nLockTime = 0;
    tx_to_spend.vin.emplace_back(COutPoint(), CScript(OP_0), 0);
    tx_to_spend.vout.emplace_back(0, challenge);

    CMutableTransaction tx_spending;
    tx_spending.version = 0;
    tx_spending.nLockTime = 0;
    tx_spending.vin.emplace_back(COutPoint(), CScript(), 0);
    tx_spending.vout.emplace_back(0, CScript(OP_RETURN));

    if (block.vtx.empty()) return std::nullopt;
    CMutableTransaction modified_coinbase{*block.vtx.at(0)};
    const int commitment_index{ReferenceWitnessCommitmentIndex(block)};
    if (commitment_index == NO_WITNESS_COMMITMENT) return std::nullopt;

    std::vector<uint8_t> signet_solution;
    CScript& witness_commitment{modified_coinbase.vout.at(commitment_index).scriptPubKey};
    if (ReferenceFetchAndClearCommitmentSection(witness_commitment, signet_solution)) {
        try {
            SpanReader reader{signet_solution};
            reader >> tx_spending.vin[0].scriptSig;
            reader >> tx_spending.vin[0].scriptWitness.stack;
            if (!reader.empty()) return std::nullopt;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    std::vector<uint8_t> block_data;
    VectorWriter writer{block_data, 0};
    writer << block.nVersion;
    writer << block.hashPrevBlock;
    writer << ReferenceModifiedMerkleRoot(modified_coinbase, block);
    writer << block.nTime;
    tx_to_spend.vin[0].scriptSig << block_data;
    tx_spending.vin[0].prevout = COutPoint(tx_to_spend.GetHash(), 0);

    return ReferenceSignetTxs{CTransaction{tx_to_spend}, CTransaction{tx_spending}};
}

std::vector<std::byte> SerializeBlock(const CBlock& block)
{
    DataStream stream;
    stream << TX_WITH_WITNESS(block);
    return {stream.begin(), stream.end()};
}

struct BlockSnapshot {
    std::vector<std::byte> serialized;
    bool f_checked;
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
    assert(block.fChecked == before.f_checked);
    assert(block.m_checked_witness_commitment == before.checked_witness_commitment);
    assert(block.m_checked_merkle_root == before.checked_merkle_root);
}

void AssertTransactionShape(const SignetTxs& actual, const ReferenceSignetTxs& expected, const CScript& challenge)
{
    assert(actual.m_to_spend == expected.to_spend);
    assert(actual.m_to_sign == expected.to_sign);

    assert(actual.m_to_spend.version == 0 && actual.m_to_spend.nLockTime == 0);
    assert(actual.m_to_spend.vin.size() == 1 && actual.m_to_spend.vout.size() == 1);
    assert(actual.m_to_spend.vin[0].prevout.IsNull());
    assert(actual.m_to_spend.vin[0].nSequence == 0);
    assert(actual.m_to_spend.vout[0].nValue == 0);
    assert(actual.m_to_spend.vout[0].scriptPubKey == challenge);

    assert(actual.m_to_sign.version == 0 && actual.m_to_sign.nLockTime == 0);
    assert(actual.m_to_sign.vin.size() == 1 && actual.m_to_sign.vout.size() == 1);
    assert(actual.m_to_sign.vin[0].prevout == COutPoint(actual.m_to_spend.GetHash(), 0));
    assert(actual.m_to_sign.vin[0].nSequence == 0);
    assert(actual.m_to_sign.vout[0].nValue == 0);
    assert(actual.m_to_sign.vout[0].scriptPubKey == CScript(OP_RETURN));
}

bool ReferenceCheckSignetBlockSolution(const CBlock& block, const Consensus::Params& consensus_params)
{
    if (block.GetHash() == consensus_params.hashGenesisBlock) return true;

    const CScript challenge{consensus_params.signet_challenge.begin(), consensus_params.signet_challenge.end()};
    const std::optional<ReferenceSignetTxs> signet_txs{ReferenceCreate(block, challenge)};
    if (!signet_txs) return false;

    PrecomputedTransactionData txdata;
    txdata.Init(signet_txs->to_sign, {signet_txs->to_spend.vout[0]});
    TransactionSignatureChecker sigcheck{
        &signet_txs->to_sign,
        0,
        signet_txs->to_spend.vout[0].nValue,
        txdata,
        MissingDataBehavior::ASSERT_FAIL};
    return VerifyScript(
        signet_txs->to_sign.vin[0].scriptSig,
        signet_txs->to_spend.vout[0].scriptPubKey,
        &signet_txs->to_sign.vin[0].scriptWitness,
        SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_NULLDUMMY,
        sigcheck);
}

} // namespace

void initialize_signet()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::SIGNET);
}

FUZZ_TARGET(signet, .init = initialize_signet)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::optional<CBlock> block = ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!block) {
        return;
    }

    const CScript challenge{ConsumeScript(fuzzed_data_provider)};
    const BlockSnapshot before{Snapshot(*block)};

    const std::optional<SignetTxs> actual_create{SignetTxs::Create(*block, challenge)};
    const std::optional<SignetTxs> repeated_create{SignetTxs::Create(*block, challenge)};
    const std::optional<ReferenceSignetTxs> expected_create{ReferenceCreate(*block, challenge)};
    assert(actual_create.has_value() == repeated_create.has_value());
    assert(actual_create.has_value() == expected_create.has_value());
    if (actual_create) {
        assert(actual_create->m_to_spend == repeated_create->m_to_spend);
        assert(actual_create->m_to_sign == repeated_create->m_to_sign);
        AssertTransactionShape(*actual_create, *expected_create, challenge);
    }
    AssertUnchanged(*block, before);

    const Consensus::Params& consensus_params{Params().GetConsensus()};
    const bool expected_check{ReferenceCheckSignetBlockSolution(*block, consensus_params)};
    const bool actual_check{CheckSignetBlockSolution(*block, consensus_params)};
    const bool repeated_check{CheckSignetBlockSolution(*block, consensus_params)};
    assert(actual_check == expected_check);
    assert(actual_check == repeated_check);
    AssertUnchanged(*block, before);
}
