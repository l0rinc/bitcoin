// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/script.h>

#include <cassert>
#include <ios>
#include <utility>
#include <vector>

static SpanReader& operator>>(SpanReader& ds, script_verify_flags& f)
{
    script_verify_flags::value_type n{0};
    ds >> n;
    f = script_verify_flags::from_int(n);
    assert(n == f.as_int());
    return ds;
}

static bool VerifyScriptWithErrorContract(const CScript& script_sig,
                                          const CScript& script_pub_key,
                                          const CScriptWitness& witness,
                                          script_verify_flags flags,
                                          const BaseSignatureChecker& checker)
{
    ScriptError error{SCRIPT_ERR_UNKNOWN_ERROR};
    const bool result{VerifyScript(script_sig, script_pub_key, &witness, flags, checker, &error)};
    assert(result == (error == SCRIPT_ERR_OK));

    const bool result_without_error{VerifyScript(script_sig, script_pub_key, &witness, flags, checker, nullptr)};
    assert(result_without_error == result);

    return result;
}

FUZZ_TARGET(script_flags)
{
    if (buffer.size() > 100'000) return;
    SpanReader ds{buffer};
    try {
        const CTransaction tx(deserialize, TX_WITH_WITNESS, ds);

        script_verify_flags verify_flags;
        ds >> verify_flags;

        assert(verify_flags == script_verify_flags::from_int(verify_flags.as_int()));

        if (!IsValidFlagCombination(verify_flags)) return;

        script_verify_flags fuzzed_flags;
        ds >> fuzzed_flags;

        std::vector<CTxOut> spent_outputs;
        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            CTxOut prevout;
            ds >> prevout;
            if (!MoneyRange(prevout.nValue)) {
                // prevouts should be consensus-valid
                prevout.nValue = 1;
            }
            spent_outputs.push_back(prevout);
        }
        PrecomputedTransactionData txdata;
        txdata.Init(tx, std::move(spent_outputs));

        for (unsigned i = 0; i < tx.vin.size(); ++i) {
            const CTxOut& prevout = txdata.m_spent_outputs.at(i);
            const TransactionSignatureChecker checker{&tx, i, prevout.nValue, txdata, MissingDataBehavior::ASSERT_FAIL};

            const bool ret = VerifyScriptWithErrorContract(tx.vin.at(i).scriptSig, prevout.scriptPubKey, tx.vin.at(i).scriptWitness, verify_flags, checker);

            // Verify that removing flags from a passing test or adding flags to a failing test does not change the result
            if (ret) {
                verify_flags &= ~fuzzed_flags;
            } else {
                verify_flags |= fuzzed_flags;
            }
            if (!IsValidFlagCombination(verify_flags)) return;

            const bool ret_fuzzed = VerifyScriptWithErrorContract(tx.vin.at(i).scriptSig, prevout.scriptPubKey, tx.vin.at(i).scriptWitness, verify_flags, checker);

            assert(ret_fuzzed == ret);
        }
    } catch (const std::ios_base::failure&) {
        return;
    }
}
