// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <addresstype.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>

FUZZ_TARGET(mixed_sigops)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    CScript redeem_script{ConsumeScript(fuzzed_data_provider)};
    redeem_script << OP_CHECKSIG;
    CScript witness_script{ConsumeScript(fuzzed_data_provider)};
    witness_script << OP_CHECKSIG;

    const COutPoint p2sh_outpoint{Txid::FromUint256(uint256::ONE), 0};
    const COutPoint p2wsh_outpoint{Txid::FromUint256(uint256::ONE), 1};
    CCoinsViewCache coins{&CoinsViewEmpty::Get(), /*deterministic=*/true};
    coins.AddCoin(p2sh_outpoint, Coin{CTxOut{0, GetScriptForDestination(ScriptHash{redeem_script})}, 0, false}, /*possible_overwrite=*/false);
    coins.AddCoin(p2wsh_outpoint, Coin{CTxOut{0, GetScriptForDestination(WitnessV0ScriptHash{witness_script})}, 0, false}, /*possible_overwrite=*/false);

    CMutableTransaction spending_tx;
    spending_tx.vin = {
        CTxIn{p2sh_outpoint, CScript{} << ToByteVector(redeem_script)},
        CTxIn{p2wsh_outpoint},
    };
    spending_tx.vin[1].scriptWitness.stack.emplace_back(witness_script.begin(), witness_script.end());
    spending_tx.vout.emplace_back(0, CScript{});
    const CTransaction tx{spending_tx};

    // Neither count depends on the verification flags, so hoist them out of the loop below.
    const int64_t legacy_sigops{GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR};
    const int64_t p2sh_sigops{GetP2SHSigOpCount(tx, coins) * WITNESS_SCALE_FACTOR};

    // SCRIPT_VERIFY_WITNESS without SCRIPT_VERIFY_P2SH violates CountWitnessSigOps' precondition.
    for (const script_verify_flags flags : {SCRIPT_VERIFY_NONE, script_verify_flags{SCRIPT_VERIFY_P2SH}, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS}) {
        int64_t expected_sigops{legacy_sigops};
        if (flags & SCRIPT_VERIFY_P2SH) {
            expected_sigops += p2sh_sigops;
        }
        for (const CTxIn& txin : tx.vin) {
            const CTxOut& prevout{coins.AccessCoin(txin.prevout).out};
            expected_sigops += CountWitnessSigOps(txin.scriptSig, prevout.scriptPubKey, txin.scriptWitness, flags);
        }
        assert(GetTransactionSigOpCost(tx, coins, flags) == expected_sigops);
    }
}
