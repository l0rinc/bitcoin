// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <crypto/sha256.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

//! Reference sigop cost computed from per-input knowledge of how the fixture
//! was constructed, independent of the aggregation conditions inside
//! GetTransactionSigOpCost(). Guards against mutations that wrongly skip P2SH
//! or witness sigop accounting for mixed-input transactions (e.g. conditioning
//! the P2SH branch on !tx.HasWitness(), or the witness loop on the absence of
//! P2SH inputs), and against the flag checks being dropped entirely.
int64_t ReferenceSigOpCost(const CTransaction& tx, const std::vector<CScript>& redeems, const std::vector<CScript>& witnesses, script_verify_flags flags)
{
    int64_t cost{0};
    for (size_t i{0}; i < tx.vin.size(); ++i) {
        cost += tx.vin[i].scriptSig.GetSigOpCount(false) * WITNESS_SCALE_FACTOR;
        if ((flags & SCRIPT_VERIFY_P2SH) && !redeems[i].empty()) {
            cost += redeems[i].GetSigOpCount(true) * WITNESS_SCALE_FACTOR;
        }
        if ((flags & SCRIPT_VERIFY_WITNESS) && !witnesses[i].empty()) {
            cost += witnesses[i].GetSigOpCount(true);
        }
    }
    for (const auto& txout : tx.vout) {
        cost += txout.scriptPubKey.GetSigOpCount(false) * WITNESS_SCALE_FACTOR;
    }
    return cost;
}

} // namespace

FUZZ_TARGET(mixed_sigops)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};

    const int n_in{fdp.ConsumeIntegralInRange<int>(2, 6)};
    CMutableTransaction mtx;
    CCoinsViewCache view{&CoinsViewEmpty::Get()};
    std::vector<CScript> redeems(n_in), witnesses(n_in);

    for (int i{0}; i < n_in; ++i) {
        const Txid txid{Txid::FromUint256(ConsumeUInt256(fdp))};
        const bool is_p2sh{fdp.ConsumeBool()};
        const int n_sigops{fdp.ConsumeIntegralInRange<int>(0, 4)};

        // A bare CHECKSIG chain is never a valid witness program (OP_CHECKSIG
        // is not a valid version byte), so a P2SH input carrying it can never
        // be reinterpreted as a P2SH-wrapped witness spend.
        CScript sig_script;
        for (int k{0}; k < n_sigops; ++k) sig_script << OP_CHECKSIG;

        CScript script_pubkey;
        CScript script_sig;
        CScriptWitness witness;
        if (is_p2sh) {
            // P2SH output: scriptSig pushes the sigop-bearing redeem script.
            const uint160 redeem_hash{Hash160(MakeUCharSpan(sig_script))};
            script_pubkey << OP_HASH160 << std::vector<uint8_t>(redeem_hash.begin(), redeem_hash.end()) << OP_EQUAL;
            script_sig << std::vector<uint8_t>(sig_script.begin(), sig_script.end());
            redeems[i] = sig_script;
        } else {
            // P2WSH output: witness carries the sigop-bearing witness script.
            uint256 program;
            CSHA256{}.Write(sig_script.data(), sig_script.size()).Finalize(program.begin());
            script_pubkey << OP_0 << std::vector<uint8_t>(program.begin(), program.end());
            witness.stack = {std::vector<uint8_t>{0x51}, std::vector<uint8_t>(sig_script.begin(), sig_script.end())};
            witnesses[i] = sig_script;
        }

        const COutPoint outpoint{txid, static_cast<uint32_t>(i)};
        view.AddCoin(outpoint, Coin{CTxOut{CAmount{1}, script_pubkey}, /*nHeightIn=*/1, /*fCoinBaseIn=*/false}, /*possible_overwrite=*/false);
        mtx.vin.emplace_back(txid, static_cast<uint32_t>(i), script_sig, /*nSequenceIn=*/0);
        mtx.vin.back().scriptWitness = witness;
    }
    mtx.vout.emplace_back(CAmount{1}, CScript{});

    const CTransaction tx{mtx};

    unsigned int p2sh_expected{0};
    for (const auto& redeem : redeems) p2sh_expected += redeem.GetSigOpCount(true);
    assert(GetP2SHSigOpCount(tx, view) == p2sh_expected);

    // SCRIPT_VERIFY_WITNESS without SCRIPT_VERIFY_P2SH is contract-invalid
    // (CountWitnessSigOps asserts on it), so only valid flag combos are checked.
    for (const auto flags : {script_verify_flags{0}, script_verify_flags{SCRIPT_VERIFY_P2SH}, script_verify_flags{SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS}}) {
        assert(GetTransactionSigOpCost(tx, view, flags) == ReferenceSigOpCost(tx, redeems, witnesses, flags));
    }
}
