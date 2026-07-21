// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/psbt.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/random.h>
#include <util/check.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using node::AnalyzePSBT;
using node::PSBTAnalysis;
using node::PSBTInputAnalysis;

static std::vector<uint8_t> SerializePSBT(const PartiallySignedTransaction& psbt)
{
    std::vector<uint8_t> serialized;
    VectorWriter{serialized, 0, psbt};
    return serialized;
}

static void AssertTransactionEnvelope(const PartiallySignedTransaction& psbt, const CMutableTransaction& tx)
{
    const auto unsigned_tx{psbt.GetUnsignedTx()};
    assert(unsigned_tx);
    assert(tx.version == unsigned_tx->version);
    assert(tx.nLockTime == unsigned_tx->nLockTime);
    assert(tx.vin.size() == unsigned_tx->vin.size());
    assert(tx.vout == unsigned_tx->vout);
    for (size_t i{0}; i < tx.vin.size(); ++i) {
        assert(tx.vin[i].prevout == unsigned_tx->vin[i].prevout);
        assert(tx.vin[i].nSequence == unsigned_tx->vin[i].nSequence);
    }
}

static void AssertExtractedTransaction(const PartiallySignedTransaction& psbt, const CMutableTransaction& tx)
{
    AssertTransactionEnvelope(psbt, tx);
    assert(tx.vin.size() == psbt.inputs.size());
    for (size_t i{0}; i < tx.vin.size(); ++i) {
        assert(tx.vin[i].scriptSig == psbt.inputs[i].final_script_sig);
        assert(tx.vin[i].scriptWitness.stack == psbt.inputs[i].final_script_witness.stack);
    }
}

static void AssertAnalysis(const PartiallySignedTransaction& psbt, const PSBTAnalysis& analysis)
{
    (void)PSBTRoleName(analysis.next);
    if (analysis.error.empty()) {
        assert(analysis.inputs.size() == psbt.inputs.size());
    } else {
        assert(analysis.inputs.empty());
        assert(analysis.next == PSBTRole::CREATOR);
        assert(!analysis.estimated_vsize);
        assert(!analysis.estimated_feerate);
        assert(!analysis.fee);
    }
}

static void AssertMergedPSBT(const PartiallySignedTransaction& base, const PartiallySignedTransaction& merged)
{
    const auto base_id{base.GetUniqueID()};
    assert(base_id);
    assert(merged.GetVersion() == base.GetVersion());
    assert(merged.GetUniqueID() == base_id);
    assert(merged.inputs.size() == base.inputs.size());
    assert(merged.outputs.size() == base.outputs.size());
}

static void AssertRemovableTransactions(const PartiallySignedTransaction& psbt)
{
    bool can_remove{true};
    for (const PSBTInput& input : psbt.inputs) {
        int witness_version;
        std::vector<unsigned char> witness_program;
        if (input.witness_utxo.IsNull() ||
            !input.witness_utxo.scriptPubKey.IsWitnessProgram(witness_version, witness_program) ||
            witness_version == 0 ||
            (input.sighash_type && (*input.sighash_type & SIGHASH_ANYONECANPAY))) {
            can_remove = false;
            break;
        }
    }
    if (can_remove) {
        for (const PSBTInput& input : psbt.inputs) {
            assert(!input.non_witness_utxo);
        }
    }
}

FUZZ_TARGET(psbt)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    auto str = fuzzed_data_provider.ConsumeRandomLengthString();
    util::Result<PartiallySignedTransaction> psbt_res = DecodeRawPSBT(MakeByteSpan(str));
    if (!psbt_res) {
        return;
    }
    PartiallySignedTransaction psbt_mut = *psbt_res;
    const PartiallySignedTransaction psbt = psbt_mut;

    // We are on purpose not forward compatible, and version 1 is disabled.
    const auto psbt_version{psbt.GetVersion()};
    Assert(psbt_version == 0 || psbt_version == 2);

    // A PSBT must roundtrip.
    std::vector<uint8_t> psbt_ser;
    VectorWriter{psbt_ser, 0, psbt};
    SpanReader reader{psbt_ser};
    PartiallySignedTransaction psbt_roundtrip(deserialize, reader);

    // And be stable across roundtrips.
    std::vector<uint8_t> roundtrip_ser;
    VectorWriter{roundtrip_ser, 0, psbt_roundtrip};
    Assert(psbt_ser == roundtrip_ser);

    const PSBTAnalysis analysis = AnalyzePSBT(psbt);
    AssertAnalysis(psbt, analysis);
    (void)PSBTRoleName(analysis.next);
    for (const PSBTInputAnalysis& input_analysis : analysis.inputs) {
        (void)PSBTRoleName(input_analysis.next);
    }

    (void)psbt.IsNull();
    (void)psbt.GetUnsignedTx();

    for (const PSBTInput& input : psbt.inputs) {
        (void)PSBTInputSigned(input);
        (void)input.IsNull();
        PSBTInput input_mod = input;
        CTxOut tx_out;
        if (input.GetUTXO(tx_out)) {
            (void)tx_out.IsNull();
            (void)tx_out.ToString();
        }
        // A PSBT input must roundtrip to signature data.
        PSBTInput input_fill{psbt_version, input_mod.prev_txid, input_mod.prev_out, input_mod.sequence};
        SignatureData sig_data;
        input_mod.FillSignatureData(sig_data);
        input_fill.FromSignatureData(sig_data);

        // Only final_script_sig and final_script_witness are filled when sigdata is complete
        if (sig_data.complete) {
            Assert(input_mod.final_script_sig == input_fill.final_script_sig);
            Assert(input_mod.final_script_witness == input_fill.final_script_witness);
        } else {
            // UTXOs don't go into SignatureData
            input_mod.non_witness_utxo.reset();
            input_mod.witness_utxo.SetNull();
            // Sighash type doesn't go into SignatureData
            input_mod.sighash_type.reset();
            // Timelocks don't go into SignatureData
            input_mod.time_locktime.reset();
            input_mod.height_locktime.reset();
            // Proprietary fields are not included in SignatureData
            input_mod.m_proprietary.clear();
            // Unknown fields are not included in SignatureData
            input_mod.unknown.clear();

            Assert(input_mod == input_fill);
        }
    }
    (void)CountPSBTUnsignedInputs(psbt);

    for (const PSBTOutput& output : psbt.outputs) {
        (void)output.IsNull();
        PSBTOutput output_mod = output;
        // A PSBT output must roundtrip to signature data.
        PSBTOutput output_fill{psbt_version, output_mod.amount, output_mod.script};
        SignatureData sig_data;
        output_mod.FillSignatureData(sig_data);
        output_fill.FromSignatureData(sig_data);

        // FillSignatureData will not fill tap tree or internal key if the tree is empty or
        // the key is not fully valid. These need to be cleared before checking for equivalence
        if (output_mod.m_tap_tree.empty() || !output_mod.m_tap_internal_key.IsFullyValid()) {
            output_mod.m_tap_tree.clear();
            std::fill(output_mod.m_tap_internal_key.begin(), output_mod.m_tap_internal_key.end(), 0);
        }
        // Sort m_tap_tree to ensure the vectors match
        std::sort(output_mod.m_tap_tree.begin(), output_mod.m_tap_tree.end());
        std::sort(output_fill.m_tap_tree.begin(), output_fill.m_tap_tree.end());
        // Proprietary fields are not included in SignatureData
        output_mod.m_proprietary.clear();
        // Unknown fields are not included in SignatureData
        output_mod.unknown.clear();

        Assert(output_mod.m_tap_internal_key == output_fill.m_tap_internal_key);
        Assert(output_mod == output_fill);
    }

    psbt_mut = psbt;
    if (FinalizePSBT(psbt_mut)) {
        const auto txdata{PrecomputePSBTData(psbt_mut)};
        assert(txdata);
        for (size_t i{0}; i < psbt_mut.inputs.size(); ++i) {
            assert(PSBTInputSignedAndVerified(psbt_mut, i, &*txdata));
        }
    }

    psbt_mut = psbt;
    CMutableTransaction result;
    if (FinalizeAndExtractPSBT(psbt_mut, result)) {
        AssertExtractedTransaction(psbt_mut, result);
        const PartiallySignedTransaction psbt_from_tx{result};
        AssertTransactionEnvelope(psbt_from_tx, result);
    }

    PartiallySignedTransaction psbt_merge = psbt;
    str = fuzzed_data_provider.ConsumeRandomLengthString();
    util::Result<PartiallySignedTransaction> psbt_merge_res = DecodeRawPSBT(MakeByteSpan(str));
    if (psbt_merge_res) {
        psbt_merge = *psbt_merge_res;
    }
    psbt_mut = psbt;
    const auto merge_before{SerializePSBT(psbt_mut)};
    if (psbt_mut.Merge(psbt_merge)) {
        AssertMergedPSBT(psbt, psbt_mut);
    } else {
        assert(SerializePSBT(psbt_mut) == merge_before);
    }
    psbt_mut = psbt;
    std::optional<PartiallySignedTransaction> comb_res = CombinePSBTs({psbt_mut, psbt_merge});
    if (comb_res) {
        AssertMergedPSBT(psbt, *comb_res);
        psbt_mut = *comb_res;
    } else {
        assert(psbt.GetVersion() != psbt_merge.GetVersion() || psbt.GetUniqueID() != psbt_merge.GetUniqueID());
    }
    for (const auto& psbt_in : psbt_merge.inputs) {
        const size_t input_count{psbt_mut.inputs.size()};
        const bool added{psbt_mut.AddInput(psbt_in)};
        assert(psbt_mut.inputs.size() == input_count + added);
    }
    for (const auto& psbt_out : psbt_merge.outputs) {
        const size_t output_count{psbt_mut.outputs.size()};
        const bool added{psbt_mut.AddOutput(psbt_out)};
        assert(psbt_mut.outputs.size() == output_count + added);
    }
    psbt_mut.unknown.insert(psbt_merge.unknown.begin(), psbt_merge.unknown.end());

    RemoveUnnecessaryTransactions(psbt_mut);
    AssertRemovableTransactions(psbt_mut);
}
