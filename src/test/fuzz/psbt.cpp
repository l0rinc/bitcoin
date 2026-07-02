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

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using node::AnalyzePSBT;
using node::PSBTAnalysis;
using node::PSBTInputAnalysis;

namespace {
std::vector<uint8_t> SerializePSBT(const PartiallySignedTransaction& psbt)
{
    std::vector<uint8_t> psbt_ser;
    VectorWriter{psbt_ser, 0, psbt};
    return psbt_ser;
}
} // namespace

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
    const std::vector<uint8_t> psbt_ser{SerializePSBT(psbt)};
    SpanReader reader{psbt_ser};
    PartiallySignedTransaction psbt_roundtrip(deserialize, reader);

    // And be stable across roundtrips.
    std::vector<uint8_t> roundtrip_ser;
    VectorWriter{roundtrip_ser, 0, psbt_roundtrip};
    Assert(psbt_ser == roundtrip_ser);

    const PSBTAnalysis analysis = AnalyzePSBT(psbt);
    (void)PSBTRoleName(analysis.next);
    Assert(analysis.estimated_vsize.has_value() == analysis.estimated_feerate.has_value());
    Assert(!analysis.estimated_feerate.has_value() || analysis.fee.has_value());
    if (analysis.error.empty()) {
        Assert(analysis.inputs.size() == psbt.inputs.size());
        Assert(analysis.next > PSBTRole::CREATOR);
    } else {
        Assert(analysis.inputs.empty());
        Assert(!analysis.estimated_vsize.has_value());
        Assert(!analysis.estimated_feerate.has_value());
        Assert(!analysis.fee.has_value());
        Assert(analysis.next == PSBTRole::CREATOR);
    }
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
    (void)FinalizePSBT(psbt_mut);

    psbt_mut = psbt;
    CMutableTransaction result;
    if (FinalizeAndExtractPSBT(psbt_mut, result)) {
        const PartiallySignedTransaction psbt_from_tx{result};
    }

    PartiallySignedTransaction psbt_merge = psbt;
    str = fuzzed_data_provider.ConsumeRandomLengthString();
    util::Result<PartiallySignedTransaction> psbt_merge_res = DecodeRawPSBT(MakeByteSpan(str));
    if (psbt_merge_res) {
        psbt_merge = *psbt_merge_res;
    }

    PartiallySignedTransaction psbt_merge_seq = psbt;
    const bool merge_res{psbt_merge_seq.Merge(psbt_merge)};
    std::optional<PartiallySignedTransaction> comb_res = CombinePSBTs({psbt, psbt_merge});
    Assert(merge_res == comb_res.has_value());
    if (comb_res) {
        Assert(SerializePSBT(psbt_merge_seq) == SerializePSBT(*comb_res));
        psbt_mut = *comb_res;
    } else {
        Assert(SerializePSBT(psbt_merge_seq) == psbt_ser);
        psbt_mut = psbt;
    }

    PartiallySignedTransaction psbt_self_merge = psbt;
    const bool self_merge_res{psbt_self_merge.Merge(psbt)};
    const std::optional<PartiallySignedTransaction> self_comb_res{CombinePSBTs({psbt, psbt})};
    Assert(self_merge_res == self_comb_res.has_value());
    if (self_comb_res) {
        Assert(SerializePSBT(psbt_self_merge) == psbt_ser);
        Assert(SerializePSBT(*self_comb_res) == psbt_ser);
    } else {
        Assert(SerializePSBT(psbt_self_merge) == psbt_ser);
    }

    for (const auto& psbt_in : psbt_merge.inputs) {
        (void)psbt_mut.AddInput(psbt_in);
    }
    for (const auto& psbt_out : psbt_merge.outputs) {
        (void)psbt_mut.AddOutput(psbt_out);
    }
    psbt_mut.unknown.insert(psbt_merge.unknown.begin(), psbt_merge.unknown.end());

    RemoveUnnecessaryTransactions(psbt_mut);
}
