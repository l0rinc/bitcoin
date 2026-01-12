// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <coins.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <key.h>
#include <pubkey.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <vector>

#include <boost/test/unit_test.hpp>

static constexpr script_verify_flags STANDARD_SCRIPT_VERIFY_FLAGS{SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_P2SH};

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
    std::vector<unsigned char> sSerialized(s.begin(), s.end());
    return sSerialized;
}

BOOST_FIXTURE_TEST_SUITE(sigopcount_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(GetSigOpCount)
{
    // Test CScript::GetSigOpCount()
    CScript s1;
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(false), 0U);
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(true), 0U);

    uint160 dummy;
    s1 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << OP_2 << OP_CHECKMULTISIG;
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(true), 2U);
    s1 << OP_IF << OP_CHECKSIG << OP_ENDIF;
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(true), 3U);
    BOOST_CHECK_EQUAL(s1.GetSigOpCount(false), 21U);

    CScript p2sh = GetScriptForDestination(ScriptHash(s1));
    CScript scriptSig;
    scriptSig << OP_0 << Serialize(s1);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(scriptSig), 3U);

    std::vector<CPubKey> keys;
    for (int i = 0; i < 3; i++)
    {
        CKey k = GenerateRandomKey();
        keys.push_back(k.GetPubKey());
    }
    CScript s2 = GetScriptForMultisig(1, keys);
    BOOST_CHECK_EQUAL(s2.GetSigOpCount(true), 3U);
    BOOST_CHECK_EQUAL(s2.GetSigOpCount(false), 20U);

    p2sh = GetScriptForDestination(ScriptHash(s2));
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(true), 0U);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(false), 0U);
    CScript scriptSig2;
    scriptSig2 << OP_1 << ToByteVector(dummy) << ToByteVector(dummy) << Serialize(s2);
    BOOST_CHECK_EQUAL(p2sh.GetSigOpCount(scriptSig2), 3U);
}

/**
 * Verifies script execution of the zeroth scriptPubKey of tx output and
 * zeroth scriptSig and witness of tx input.
 */
static ScriptError VerifyWithFlag(const CTransaction& output, const CMutableTransaction& input, script_verify_flags flags)
{
    ScriptError error;
    CTransaction inputi(input);
    bool ret = VerifyScript(inputi.vin[0].scriptSig, output.vout[0].scriptPubKey, &inputi.vin[0].scriptWitness, flags, TransactionSignatureChecker(&inputi, 0, output.vout[0].nValue, MissingDataBehavior::ASSERT_FAIL), &error);
    BOOST_CHECK((ret == true) == (error == SCRIPT_ERR_OK));

    return error;
}

/**
 * Builds a creationTx from scriptPubKey and a spendingTx from scriptSig
 * and witness such that spendingTx spends output zero of creationTx.
 * Also inserts creationTx's output into the coins view.
 */
static void BuildTxs(CMutableTransaction& spendingTx, CCoinsViewCache& coins, CMutableTransaction& creationTx, const CScript& scriptPubKey, const CScript& scriptSig, const CScriptWitness& witness)
{
    creationTx.version = 1;
    creationTx.vin.resize(1);
    creationTx.vin[0].prevout.SetNull();
    creationTx.vin[0].scriptSig = CScript();
    creationTx.vout.resize(1);
    creationTx.vout[0].nValue = 1;
    creationTx.vout[0].scriptPubKey = scriptPubKey;

    spendingTx.version = 1;
    spendingTx.vin.resize(1);
    spendingTx.vin[0].prevout.hash = creationTx.GetHash();
    spendingTx.vin[0].prevout.n = 0;
    spendingTx.vin[0].scriptSig = scriptSig;
    spendingTx.vin[0].scriptWitness = witness;
    spendingTx.vout.resize(1);
    spendingTx.vout[0].nValue = 1;
    spendingTx.vout[0].scriptPubKey = CScript();

    AddCoins(coins, CTransaction(creationTx), 0);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost_Multisig)
{
    // Multisig script (legacy counting)
    CMutableTransaction creationTx;
    CMutableTransaction spendingTx;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();

    CScript scriptPubKey = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
    // Do not use a valid signature to avoid using wallet operations.
    CScript scriptSig = CScript() << OP_0 << OP_0;

    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, CScriptWitness());
    // Legacy counting only includes signature operations in scriptSigs and scriptPubKeys
    // of a transaction and does not take the actual executed sig operations into account.
    // spendingTx in itself does not contain a signature operation.
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 0);
    // creationTx contains two signature operations in its scriptPubKey, but legacy counting
    // is not accurate.
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(creationTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), MAX_PUBKEYS_PER_MULTISIG * WITNESS_SCALE_FACTOR);
    // Sanity check: script verification fails because of an invalid signature.
    BOOST_CHECK_EQUAL(VerifyWithFlag(CTransaction(creationTx), spendingTx, STANDARD_SCRIPT_VERIFY_FLAGS), SCRIPT_ERR_CHECKMULTISIGVERIFY);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost_MultisigP2SH)
{
    // Multisig nested in P2SH
    CMutableTransaction creationTx;
    CMutableTransaction spendingTx;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();

    CScript redeemScript = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
    CScript scriptSig = CScript() << OP_0 << OP_0 << ToByteVector(redeemScript);

    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, CScriptWitness());
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 2 * WITNESS_SCALE_FACTOR);
    BOOST_CHECK_EQUAL(VerifyWithFlag(CTransaction(creationTx), spendingTx, STANDARD_SCRIPT_VERIFY_FLAGS), SCRIPT_ERR_CHECKMULTISIGVERIFY);

    // P2SH sigops are not counted if we don't set the SCRIPT_VERIFY_P2SH flag
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, /*flags=*/0), 0);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost_P2WPKH)
{
    // P2WPKH witness program
    CMutableTransaction creationTx;
    CMutableTransaction spendingTx;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();

    CScript scriptPubKey = GetScriptForDestination(WitnessV0KeyHash(pubkey));
    CScript scriptSig = CScript();
    CScriptWitness scriptWitness;
    scriptWitness.stack.emplace_back(0);
    scriptWitness.stack.emplace_back(0);

    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 1);
    // No signature operations if we don't verify the witness.
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS & ~SCRIPT_VERIFY_WITNESS), 0);
    BOOST_CHECK_EQUAL(VerifyWithFlag(CTransaction(creationTx), spendingTx, STANDARD_SCRIPT_VERIFY_FLAGS), SCRIPT_ERR_EQUALVERIFY);

    // The sig op cost for witness version != 0 is zero.
    BOOST_REQUIRE_EQUAL(scriptPubKey[0], OP_0);
    scriptPubKey[0] = OP_1;
    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 0);
    scriptPubKey[0] = OP_0;
    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);

    // The witness of a coinbase transaction is not taken into account.
    spendingTx.vin[0].prevout.SetNull();
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 0);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost_P2WPKH_P2SH)
{
    // P2WPKH nested in P2SH
    CMutableTransaction creationTx;
    CMutableTransaction spendingTx;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();

    CScript scriptSig = GetScriptForDestination(WitnessV0KeyHash(pubkey));
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(scriptSig));
    scriptSig = CScript() << ToByteVector(scriptSig);
    CScriptWitness scriptWitness;
    scriptWitness.stack.emplace_back(0);
    scriptWitness.stack.emplace_back(0);

    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 1);
    BOOST_CHECK_EQUAL(VerifyWithFlag(CTransaction(creationTx), spendingTx, STANDARD_SCRIPT_VERIFY_FLAGS), SCRIPT_ERR_EQUALVERIFY);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost_P2WSH)
{
    // P2WSH witness program
    CMutableTransaction creationTx;
    CMutableTransaction spendingTx;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();

    CScript witnessScript = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
    CScript scriptPubKey = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
    CScript scriptSig = CScript();
    CScriptWitness scriptWitness;
    scriptWitness.stack.emplace_back(0);
    scriptWitness.stack.emplace_back(0);
    scriptWitness.stack.emplace_back(witnessScript.begin(), witnessScript.end());

    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 2);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS & ~SCRIPT_VERIFY_WITNESS), 0);
    BOOST_CHECK_EQUAL(VerifyWithFlag(CTransaction(creationTx), spendingTx, STANDARD_SCRIPT_VERIFY_FLAGS), SCRIPT_ERR_CHECKMULTISIGVERIFY);
}

BOOST_AUTO_TEST_CASE(GetTxSigOpCost_P2WSH_P2SH)
{
    // P2WSH nested in P2SH
    CMutableTransaction creationTx;
    CMutableTransaction spendingTx;

    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    CKey key = GenerateRandomKey();
    CPubKey pubkey = key.GetPubKey();

    CScript witnessScript = CScript() << 1 << ToByteVector(pubkey) << ToByteVector(pubkey) << 2 << OP_CHECKMULTISIGVERIFY;
    CScript redeemScript = GetScriptForDestination(WitnessV0ScriptHash(witnessScript));
    CScript scriptPubKey = GetScriptForDestination(ScriptHash(redeemScript));
    CScript scriptSig = CScript() << ToByteVector(redeemScript);
    CScriptWitness scriptWitness;
    scriptWitness.stack.emplace_back(0);
    scriptWitness.stack.emplace_back(0);
    scriptWitness.stack.emplace_back(witnessScript.begin(), witnessScript.end());

    BuildTxs(spendingTx, coins, creationTx, scriptPubKey, scriptSig, scriptWitness);
    BOOST_CHECK_EQUAL(GetTransactionSigOpCost(CTransaction(spendingTx), coins, STANDARD_SCRIPT_VERIFY_FLAGS), 2);
    BOOST_CHECK_EQUAL(VerifyWithFlag(CTransaction(creationTx), spendingTx, STANDARD_SCRIPT_VERIFY_FLAGS), SCRIPT_ERR_CHECKMULTISIGVERIFY);
}

BOOST_AUTO_TEST_SUITE_END()
