// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <addresstype.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <key.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <test/util/transaction_utils.h>
#include <tinyformat.h>
#include <univalue.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <validation.h>

#include <ranges>
#include <string>

using namespace util::hex_literals;

BOOST_AUTO_TEST_SUITE(bip54_tests)

// Uncomment to output updated JSON tests.
// #define UPDATE_JSON_TESTS

#ifdef UPDATE_JSON_TESTS
/** Write a list of test vectors to disk as JSON. */
template<typename T>
static void WriteJSONTestVectors(const std::vector<T>& test_vectors, std::string_view filename)
{
    UniValue json_vectors{UniValue::VARR};
    for (const auto& test_vector: test_vectors) {
        json_vectors.push_back(test_vector.GetJson());
    }
    auto json_str{json_vectors.write(4)};
    json_str += '\n';
    FILE* file = fsbridge::fopen(filename, "w");
    fputs(json_str.c_str(), file);
    fclose(file);
}
#endif

/** A test vector for the per-transaction sigop limit in BIP54. */
struct TestVectorSigops {
    //! The transaction being evaluated.
    const CTransaction tx;
    //! The outputs corresponding to the transaction's inputs.
    const std::vector<CTxOut> spent_outputs;
    //! Whether this transaction passes the BIP54 sigops check.
    const bool valid;
    //! Description of the test vector.
    const std::string comment;

    explicit TestVectorSigops(CTransaction spending_tx, std::vector<CTxOut> spent_txos, bool success, std::string com):
        tx{std::move(spending_tx)}, spent_outputs{std::move(spent_txos)}, valid{success}, comment{std::move(com)} {}

    UniValue GetJson() const
    {
        UniValue json{UniValue::VOBJ}, spent_txos{UniValue::VARR};
        for (const auto& txo: spent_outputs) {
            spent_txos.push_back(HexStr(DataStream{} << txo));
        }
        json.pushKV("comment", comment);
        json.pushKV("spent_outputs", std::move(spent_txos));
        json.pushKV("tx", EncodeHexTx(tx));
        json.pushKV("valid", valid);
        return json;
    }
};

/** Get the (non-extended) child private key at the provided derivation index. */
static CKey GetKeyAt(const CExtKey& parent_xprv, unsigned idx)
{
    CExtKey child_xprv;
    Assert(parent_xprv.Derive(child_xprv, idx));
    return child_xprv.key;
}

/** Generate an ECDSA signature for a specified input. */
static std::vector<uint8_t> SignInput(const CKey& key, const CScript& spent_script, CMutableTransaction& tx, unsigned idx, unsigned type = SIGHASH_ALL)
{
    const CAmount dummy{0};
    std::vector<uint8_t> sig;
    const auto sighash{SignatureHash(spent_script, tx, idx, type, dummy, SigVersion::BASE)};
    Assert(key.Sign(sighash, sig));
    sig.push_back(static_cast<uint8_t>(type));
    return sig;
}

/** Verify a transaction input's script against consensus rules. Segwit inputs need the amount, Taproot ones all spent outputs. */
static bool VerifyTxin(const CScript& spent_script, const CMutableTransaction& tx, unsigned idx, CAmount amount = 0, std::vector<CTxOut> spent_outputs = {})
{
    Assert(idx < tx.vin.size());
    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::move(spent_outputs), /*force=*/false);
    const MutableTransactionSignatureChecker checker{&tx, idx, amount, txdata, MissingDataBehavior::ASSERT_FAIL};
    return VerifyScript(tx.vin[idx].scriptSig, spent_script, &tx.vin[idx].scriptWitness, MANDATORY_SCRIPT_VERIFY_FLAGS, checker);
}

/** Get a list of all coins spent by this transaction. All coins must be in cache. */
template<typename T>
static std::vector<CTxOut> RecordSpent(const CCoinsViewCache& coins, const T& tx)
{
    std::vector<CTxOut> spent_outputs;
    spent_outputs.reserve(tx.vin.size());
    for (const auto& txin: tx.vin) {
        spent_outputs.push_back(Assert(coins.GetCoin(txin.prevout))->out);
    }
    return spent_outputs;
}

/** Add a coin paying to this script to the cache, and an input spending it to the transaction. */
static void AddSpentCoin(CCoinsViewCache& coins, CMutableTransaction& tx, CAmount value, const CScript& script_pubkey)
{
    CMutableTransaction tx_create;
    tx_create.vout.emplace_back(value, script_pubkey);
    AddCoins(coins, CTransaction{tx_create}, /*nHeight=*/0);
    tx.vin.emplace_back(tx_create.GetHash(), 0);
}

/** Check this transaction does not exceed the BIP54 sigops limit, and record it as a test vector. */
static void CheckWithinBIP54Limits(CTransaction&& tx, const CCoinsViewCache& coins, std::vector<TestVectorSigops>& test_vectors, std::string comment)
{
    BOOST_CHECK_MESSAGE(Consensus::CheckSigopsBIP54(tx, coins), comment);

    auto spent_outputs{RecordSpent(coins, tx)};
    test_vectors.emplace_back(std::move(tx), std::move(spent_outputs), /*valid=*/true, std::move(comment));
}

/** Check this transaction exceeds the BIP54 sigops limit, and record it as a test vector. */
static void CheckExceedsBIP54Limits(CTransaction&& tx, const CCoinsViewCache& coins, std::vector<TestVectorSigops>& test_vectors, std::string comment)
{
    BOOST_CHECK_MESSAGE(!Consensus::CheckSigopsBIP54(tx, coins), comment);

    auto spent_outputs{RecordSpent(coins, tx)};
    test_vectors.emplace_back(std::move(tx), std::move(spent_outputs), /*valid=*/false, std::move(comment));
}

/**
 * Test the BIP54 per-transaction limit on legacy signature operations in inputs. We perform
 * extensive tests of the new limit from a few different perspectives. These extensive tests will
 * also be used to generate test vectors to help validate the BIP's semantics and re-implementation
 * of this logic. There are broadly 3 categories to this test. First, we check the bounds of the
 * new limit under various semi-realistic conditions: valid transactions with different combinations
 * of inputs and outputs types. Then, we exercise the new limit under some historical block chain
 * transactions known to be BIP54-invalid. Finally, we exercise some specific details and edge
 * cases of the implementation.
 */
BOOST_AUTO_TEST_CASE(bip54_legacy_sigops)
{
    ECC_Context ecc_ctx;
    // All keys in this test are derived from this seed using BIP32.
    CExtKey xprv;
    xprv.SetSeed("50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0"_hex);

    // For all following test cases we will use a transaction with outputs of various types.
    CMutableTransaction tx;
    tx.vout.emplace_back(0, GetScriptForDestination(PubKeyDestination(GetKeyAt(xprv, 1).GetPubKey())));
    tx.vout.emplace_back(0, GetScriptForDestination(PKHash(GetKeyAt(xprv, 2).GetPubKey())));
    const auto ms_script{CScript{} << OP_2 << ToByteVector(GetKeyAt(xprv, 2).GetPubKey()) << ToByteVector(GetKeyAt(xprv, 3).GetPubKey())
                         << ToByteVector(GetKeyAt(xprv, 4).GetPubKey()) << OP_3 << OP_CHECKMULTISIG};
    tx.vout.emplace_back(0, ms_script);
    tx.vout.emplace_back(0, GetScriptForDestination(ScriptHash(ms_script)));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessV0KeyHash(GetKeyAt(xprv, 5).GetPubKey())));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessV0ScriptHash(ms_script)));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey{GetKeyAt(xprv, 6).GetPubKey()})));
    tx.vout.emplace_back(0, GetScriptForDestination(PayToAnchor()));
    tx.vout.emplace_back(0, GetScriptForDestination(WitnessUnknown(8, {42, 42, 42})));
    Assert(tx.vout.size() == std::variant_size_v<CTxDestination>);

    // Record the test vectors.
    std::vector<TestVectorSigops> test_vectors;

    // Reach the 2'500 limit using only CHECKSIG's in a bare Script.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Use the first derivation as the private key to use in spent coins.
        const CKey privkey{GetKeyAt(xprv, 0)};
        const auto pubkey{privkey.GetPubKey()};

        // Create a spent Script that accounts for exactly a hundred sigops.
        auto spent_script{CScript() << ToByteVector(pubkey)};
        for (int i{0}; i < 99; ++i) {
            spent_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        spent_script << OP_CHECKSIG;

        // Reach 2500 sigops: one more sigop and we'll exceed the limit.
        for (int i{0}; i < 25; ++i) {
            AddSpentCoin(coins, tx_copy, i, spent_script);
        }

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(privkey, spent_script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2500 CHECKSIGs");

        // Add one more input with a single CHECKSIG.
        auto spent_script2{CScript() << ToByteVector(pubkey) << OP_CHECKSIG};
        const auto idx{tx_copy.vin.size()};
        AddSpentCoin(coins, tx_copy, idx, spent_script2);

        // Sign it and make sure it's valid.
        tx_copy.vin.back().scriptSig = CScript{} << SignInput(privkey, spent_script2, tx_copy, idx);
        BOOST_REQUIRE(VerifyTxin(spent_script2, tx_copy, idx));

        // Resign the previous inputs as we just invalidated the signature.
        for (size_t i{0}; i < tx_copy.vin.size() - 1; ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, spent_script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // Now we bump into the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2501 CHECKSIGs");

        // Now malleate a bunch of unrelated fields to demonstrate how changing those does not affect
        // the BIP54 sigops calculation.
        tx_copy.version = 42;
        tx_copy.nLockTime = 21;
        tx_copy.vout = {tx_copy.vout.begin() + 2, tx_copy.vout.end()};
        tx_copy.vout[0].nValue = 50;
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].nSequence = (84 * i) | CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG;
        }

        // Resign inputs as we just invalidated the signatures.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            const bool last_txin{i == tx_copy.vin.size() - 1};
            const CScript& script{last_txin ? spent_script2 : spent_script};
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(script, tx_copy, i));
        }

        // The number of accounted sigops hasn't changed. We still exceed the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2501 CHECKSIGs and unrelated transaction fields malleated");

        // Drop the last input with the single CHECKSIG, and resign everything (since we use SIGHASH_ALL).
        tx_copy.vin.pop_back();
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, spent_script, tx_copy, i);
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // Now we don't exceed the limit anymore.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 2500 CHECKSIGs and unrelated transaction fields malleated");
    }

    // Reach the 2'500 limit using only CHECKSIG's in a P2SH redeemScript.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Use the second derivation as the private key to use in spent coins.
        const CKey privkey{GetKeyAt(xprv, 1)};
        const auto pubkey{privkey.GetPubKey()};

        // Create a redeem Script that accounts for exactly a hundred sigops.
        auto redeem_script{CScript() << ToByteVector(pubkey)};
        for (int i{0}; i < 99; ++i) {
            redeem_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        redeem_script << OP_CHECKSIG;
        const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

        // Reach 2500 sigops: one more sigop and we'll exceed the limit. Contrary
        // to the bare Script version, here we'll use a single creation tx.
        CMutableTransaction tx_create;
        for (int i{0}; i < 25; ++i) {
            tx_create.vout.emplace_back(i, spk);
        }
        const auto prev_txid{tx_create.GetHash()};
        for (int i{0}; i < 25; ++i) {
            tx_copy.vin.emplace_back(prev_txid, i);
        }
        AddCoins(coins, CTransaction(tx_create), 0, false);

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(privkey, redeem_script, tx_copy, i) << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "P2SH inputs totalling 2500 CHECKSIGs");

        // Add one more input with a single CHECKSIG (a bare P2PK, to mix input types).
        auto spent_script{CScript() << ToByteVector(pubkey) << OP_CHECKSIG};
        const auto idx{tx_copy.vin.size()};
        AddSpentCoin(coins, tx_copy, idx, spent_script);

        // Sign it and make sure it's valid.
        tx_copy.vin.back().scriptSig << SignInput(privkey, spent_script, tx_copy, idx);
        BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, idx));

        // Now we bump into the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "P2SH inputs totalling 2500 CHECKSIGs + 1 P2PK input");
    }

    // Create a transaction spending 250 7-of-10 bare multisigs with 10 different public keys.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Get 10 private keys to create the multisig.
        std::vector<CKey> privkeys;
        for (int i{0}; i < 10; ++i) {
            privkeys.push_back(GetKeyAt(xprv, 10 + i));
        }

        // A 7-of-10 multisig.
        auto spent_script{CScript() << OP_7};
        for (const auto& pk: privkeys) {
            spent_script << ToByteVector(pk.GetPubKey());
        }
        spent_script << OP_10 << OP_CHECKMULTISIG;

        // Spend 250 of those in a single transaction, reaching the 2500 sigops limit.
        for (int i{0}; i < 250; ++i) {
            AddSpentCoin(coins, tx_copy, i, spent_script);
        }

        // Sign each input. Make sure all transaction inputs are valid. Sign using ACP so we
        // don't invalidate the signatures when adding an input below.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << OP_0;
            for (const auto& pk: privkeys | std::views::take(7)) {
                tx_copy.vin[i].scriptSig << SignInput(pk, spent_script, tx_copy, i, SIGHASH_ALL | SIGHASH_ANYONECANPAY);
            }
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 250 7-of-10 CHECKMULTISIGs");

        // Add a 1-of-1 CHECKMULTISIG input.
        auto single_spent_script{CScript{} << OP_1 << ToByteVector(privkeys.front().GetPubKey()) << OP_1 << OP_CHECKMULTISIG};
        const auto idx{tx_copy.vin.size()};
        AddSpentCoin(coins, tx_copy, idx, single_spent_script);

        // Sign the 1-of-1 CMS.
        tx_copy.vin.back().scriptSig << OP_0 << SignInput(privkeys.front(), single_spent_script, tx_copy, idx);
        BOOST_REQUIRE(VerifyTxin(single_spent_script, tx_copy, idx));

        // Now we do exceed the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 250 7-of-10 CHECKMULTISIGs + 1 1-of-1 CHECKMULTISIG");
    }

    // Create a transaction spending 125 16-of-17 bare multisigs. This demonstrates how
    // a pubkey count >16 will be accounted as 20 keys. This also repeats the same public
    // key and does not use ACP.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Get a single private key to repeat in the multisig.
        const auto privkey{GetKeyAt(xprv, 20)};
        const auto pubkey{privkey.GetPubKey()};

        // A 16-of-17 multisig with the same key repeated 17 times.
        auto spent_script{CScript() << OP_16};
        for (int i{0}; i < 17; ++i) {
            spent_script << ToByteVector(pubkey);
        }
        spent_script << 17 << OP_CHECKMULTISIG;

        // Spend 125 of those in a single transaction, reaching the 2500 sigops limit.
        for (int i{0}; i < 125; ++i) {
            AddSpentCoin(coins, tx_copy, i, spent_script);
        }

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << OP_0;
            const auto sig{SignInput(privkey, spent_script, tx_copy, i)};
            for (int j{0}; j < 16; ++j) {
                tx_copy.vin[i].scriptSig << sig;
            }
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 125 16-of-17 CHECKMULTISIGs");

        // Add one more input with a single sigop (a P2PKH to mix input types).
        auto spk{GetScriptForDestination(PKHash(pubkey))};
        const auto idx{tx_copy.vin.size()};
        AddSpentCoin(coins, tx_copy, idx, spk);

        // Sign it and make sure it's valid.
        tx_copy.vin.back().scriptSig << SignInput(privkey, spk, tx_copy, idx) << ToByteVector(pubkey);
        BOOST_REQUIRE(VerifyTxin(spk, tx_copy, idx));

        // Resign all previous inputs which were invalidated by adding a new input.
        for (size_t i{0}; i < idx; ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << OP_0;
            const auto sig{SignInput(privkey, spent_script, tx_copy, i)};
            for (int j{0}; j < 16; ++j) {
                tx_copy.vin[i].scriptSig << sig;
            }
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, i));
        }

        // Now we bump into the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Bare Script inputs totalling 125 16-of-17 CHECKMULTISIGs + 1 P2PKH input");
    }

    // Exceed the 2'500 limit using 18-of-18's CHECKMULTISIGs in an intentionally contrived P2SH redeemScript.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // Use a single private key, we'll copy the key to create the multisigs.
        const CKey privkey{GetKeyAt(xprv, 30)};
        const auto pubkey{privkey.GetPubKey()};

        // Create a redeem script performing two 18-of-18 CHECKMULTISIG's.
        auto redeem_script{CScript() << ToByteVector(pubkey)};
        // From a stack `<sig> <pk>`, create `<sig> <pk> <> {<sig>}*18 <pk>`
        redeem_script << OP_DUP << OP_TOALTSTACK << OP_OVER << OP_0 << OP_SWAP << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_FROMALTSTACK;
        // From a stack `<sig> <pk> <> {<sig>}*18 <pk>`, create `<sig> <pk> <> {<sig>}*18 <18> {<pk>}*18 <18> CMSVERIFY`
        redeem_script << 18 << OP_SWAP << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << 18 << OP_CHECKMULTISIGVERIFY;
        // From a stack `<sig> <pk>`, create `<> {<sig>}*18 <pk>`
        redeem_script << OP_0 << OP_ROT << OP_ROT << OP_TOALTSTACK << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_FROMALTSTACK;
        // From a stack `<> {<sig>}*18 <pk>`, create `<> {<sig>}*18 <18> {<pk>}*18 <18> CHECKMULTISIG`
        redeem_script << 18 << OP_SWAP << OP_DUP << OP_2DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << 18 << OP_CHECKMULTISIG;
        const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

        // Reach 2480 sigops with 62 inputs.
        CMutableTransaction tx_create;
        for (int i{0}; i < 62; ++i) {
            tx_create.vout.emplace_back(i, spk);
        }
        const auto prev_txid{tx_create.GetHash()};
        for (int i{0}; i < 62; ++i) {
            tx_copy.vin.emplace_back(prev_txid, i);
        }
        AddCoins(coins, CTransaction(tx_create), 0, false);

        // Sign each input. Make sure all transaction inputs are valid.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(privkey, redeem_script, tx_copy, i) << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy, i));
        }

        // We don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "62 inputs with a contrived P2SH redeemScript executing 2 18-of-18 CHECKMULTISIGs");

        // Add one more input with the same spent script.
        const auto idx{tx_copy.vin.size()};
        tx_create.vout.emplace_back(idx, spk);
        AddCoins(coins, CTransaction(tx_create), 0, false);
        tx_copy.vin.emplace_back(tx_create.GetHash(), idx);

        // Re-sign each input.
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig = CScript{} << SignInput(privkey, redeem_script, tx_copy, i) << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy, i));
        }

        // We now reached 2520 sigops. We exceed the limit.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "63 inputs with a contrived P2SH redeemScript executing 2 18-of-18 CHECKMULTISIGs");
    }

    // Now reach exactly 2500 sigops with a transaction mixing CMS-only input, CHECKSIG-only input, an input with
    // a mix of both, and 3 more of the same but under P2SH. Then we'll check we do exceed the limit by adding one
    // more legacy sigop, but not by adding non-legacy sigops for various existing non-legacy input types. This
    // test also demonstrates how sigops in unexecuted branches still account toward the limit.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // A script with 100 CHECKSIG's which takes a single sig as input.
        const auto cs_only_privkey{GetKeyAt(xprv, 40)};
        const auto cs_only_pubkey{cs_only_privkey.GetPubKey()};
        auto cs_only_script{CScript{} << ToByteVector(cs_only_pubkey)};
        for (int i{0}; i < 99; ++i) {
            cs_only_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        cs_only_script << OP_CHECKSIG;

        // A script with 9 1-of-20 CHECKMULTISIG's which takes 9 sigs as input.
        CScript cms_only_barescript;
        for (int i{0}; i < 9; ++i) {
            const auto privkey{GetKeyAt(xprv, 41 + i)};
            const auto pubkey{privkey.GetPubKey()};
            cms_only_barescript << OP_0 << OP_SWAP << OP_1;
            for (int j{0}; j < 20; ++j) cms_only_barescript << ToByteVector(pubkey);
            cms_only_barescript << 20 << (i < 9 - 1 ? OP_CHECKMULTISIGVERIFY : OP_CHECKMULTISIG);
        }

        // A script with 50 CHECKSIG's and 5 19-of-19 CHECKMULTISIG's which takes as input
        // one signature for all CHECKSIG's and then the inputs to the 5 CMS.
        const auto mixed_bare_privkey{GetKeyAt(xprv, 50)};
        const auto mixed_bare_pubkey{mixed_bare_privkey.GetPubKey()};
        auto mixed_barescript{CScript{} << ToByteVector(mixed_bare_pubkey)};
        for (int i{0}; i < 49; ++i) {
            mixed_barescript << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        mixed_barescript << OP_CHECKSIGVERIFY;
        for (int i{0}; i < 5; ++i) {
            mixed_barescript << 19;
            for (int j{0}; j < 19; ++j) mixed_barescript << ToByteVector(mixed_bare_pubkey);
            mixed_barescript << 19 << (i < 5 - 1 ? OP_CHECKMULTISIGVERIFY : OP_CHECKMULTISIG);
        }

        // A P2SH with 100 CHECKSIG's.
        const auto cs_only_p2sh{GetScriptForDestination(ScriptHash(cs_only_script))};

        // A P2SH with 8 16-of-16 CHECKMULTISIG's. Takes the inputs to all 8 CMS.
        CScript cms_only_redeem_script;
        for (int i{0}; i < 8; ++i) {
            const auto privkey{GetKeyAt(xprv, 51 + i)};
            const auto pubkey{privkey.GetPubKey()};
            cms_only_redeem_script << OP_16 << ToByteVector(pubkey) << OP_DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_16;
            cms_only_redeem_script << (i < 8 - 1 ? OP_CHECKMULTISIGVERIFY : OP_CHECKMULTISIG);
        }
        const auto cms_only_p2sh{GetScriptForDestination(ScriptHash(cms_only_redeem_script))};

        // A P2SH with 9 13-of-13 CMS followed by 5 CHECKSIGs. The 9 CHECKMULTISIG's are in
        // a non-executed Script branch. Takes as input a single signature for the CHECKSIG's.
        const auto mixed_p2sh_privkey{GetKeyAt(xprv, 50)};
        const auto mixed_p2sh_pubkey{mixed_p2sh_privkey.GetPubKey()};
        auto mixed_redeem_script{CScript{} << OP_0 << OP_IF};
        for (int i{0}; i < 9; ++i) {
            mixed_redeem_script << OP_13 << ToByteVector(mixed_p2sh_pubkey) << OP_DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_13 << OP_CHECKMULTISIGVERIFY;
        }
        mixed_redeem_script << OP_ENDIF << ToByteVector(mixed_p2sh_pubkey);
        for (int i{0}; i < 4; ++i) {
            mixed_redeem_script << OP_2DUP << OP_CHECKSIGVERIFY;
        }
        mixed_redeem_script << OP_CHECKSIG;
        const auto mixed_p2sh{GetScriptForDestination(ScriptHash(mixed_redeem_script))};

        // Now create the spending transaction. To start it will spend 2 inputs with a
        // bare script filled with CHECKSIG's. That's 200 sigops accounted for.
        for (int i{0}; i < 2; ++i) {
            AddSpentCoin(coins, tx_copy, i, cs_only_script);
        }

        // Sign those two first inputs. (With ACP so we can add more inputs.)
        for (size_t i{0}; i < tx_copy.vin.size(); ++i) {
            tx_copy.vin[i].scriptSig << SignInput(cs_only_privkey, cs_only_script, tx_copy, i, SIGHASH_ALL | SIGHASH_ANYONECANPAY);
            BOOST_REQUIRE(VerifyTxin(cs_only_script, tx_copy, i));
        }

        // Then it will spend 10 inputs with bare multisigs, bringing it to 2000 sigops
        // accounted for in total.
        {
            CMutableTransaction tx_create;
            for (int i{0}; i < 10; ++i) {
                tx_create.vout.emplace_back(i, cms_only_barescript);
            }
            const auto prev_txid{tx_create.GetHash()};
            for (int i{0}; i < 10; ++i) {
                tx_copy.vin.emplace_back(prev_txid, i);
            }
            AddCoins(coins, CTransaction(tx_create), 0, false);
        }

        // Sign those ten additional inputs. (With ACP so we can add more inputs.)
        for (size_t i{2}; i < tx_copy.vin.size(); ++i) {
            for (size_t j{0}; j < 9; ++j) {
                // We used indexes from 41 through 49 for the 9 CMS.
                const auto privkey{GetKeyAt(xprv, 49 - j)};
                const auto sig{SignInput(privkey, cms_only_barescript, tx_copy, i, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
                tx_copy.vin[i].scriptSig << sig;
            }
            BOOST_REQUIRE(VerifyTxin(cms_only_barescript, tx_copy, i));
        }

        // And it will spend one input for a bare Script with mixed CMS and CHECKSIGs. That's
        // now 2150 sigops accounted in total.
        AddSpentCoin(coins, tx_copy, 422421, mixed_barescript);

        // Sign this input (still ACP). Use ops in the scriptSig for a change.
        {
            const auto idx{tx_copy.vin.size() - 1};
            const auto sig{SignInput(mixed_bare_privkey, mixed_barescript, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
            for (size_t i{0}; i < 5; ++i) {
                tx_copy.vin[idx].scriptSig << OP_0 << sig << OP_DUP << OP_2DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP << OP_3DUP;
            }
            tx_copy.vin[idx].scriptSig << sig; // For the CHECKSIG's
            BOOST_REQUIRE(VerifyTxin(mixed_barescript, tx_copy, idx));
        }

        // It will spend a single p2sh input with only CHECKSIGs, getting to 2250 sigops.
        AddSpentCoin(coins, tx_copy, 181827, cs_only_p2sh);

        // Sign this input (still ACP).
        {
            const auto idx{tx_copy.vin.size() - 1};
            const auto sig{SignInput(cs_only_privkey, cs_only_script, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
            tx_copy.vin[idx].scriptSig << sig << ToByteVector(cs_only_script);
            BOOST_REQUIRE(VerifyTxin(cs_only_p2sh, tx_copy, idx));
        }

        // It will spend a single p2sh input only CMS's, getting to 2378 sigops.
        AddSpentCoin(coins, tx_copy, 999, cms_only_p2sh);

        // Sign the P2SH multisigs, still with ACP.
        {
            const auto idx{tx_copy.vin.size() - 1};
            for (int i{0}; i < 8; ++i) {
                // Traverse the derivation indexes used to create them (51 through 58) in reverse order.
                const auto privkey{GetKeyAt(xprv, 58 - i)};
                const auto sig{SignInput(privkey, cms_only_redeem_script, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
                tx_copy.vin[idx].scriptSig << OP_0;
                for (int j{0}; j < 16; ++j) {
                    tx_copy.vin[idx].scriptSig << sig;
                }
            }
            tx_copy.vin[idx].scriptSig << ToByteVector(cms_only_redeem_script);
            BOOST_REQUIRE(VerifyTxin(cms_only_p2sh, tx_copy, idx));
        }

        // It will finally spend a single p2sh input with mixed CS - CMS, getting
        // to 2500 sigops.
        AddSpentCoin(coins, tx_copy, 776, mixed_p2sh);

        // Sign this final input. It only needs one signature for the CHECKSIG's because
        // the CMS are not executed.
        {
            const auto idx{tx_copy.vin.size() - 1};
            const auto sig{SignInput(mixed_p2sh_privkey, mixed_redeem_script, tx_copy, idx, SIGHASH_ALL | SIGHASH_ANYONECANPAY)};
            tx_copy.vin[idx].scriptSig << sig << ToByteVector(mixed_redeem_script);
            BOOST_REQUIRE(VerifyTxin(mixed_p2sh, tx_copy, idx));
        }

        // We reached exactly 2500 sigops, we don't exceed the limit.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops");

        // Now we are going to add an input and make sure we exceed the limit or not as
        // expected. This is the index of this input.
        const auto idx{tx_copy.vin.size()};

        // Adding a P2PK input will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 60)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spent_script{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};

            AddSpentCoin(coins, tx_copy2, 10101, spent_script);

            const auto sig{SignInput(privkey, spent_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2PK input");
        }

        // Adding a P2PKH input will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 60)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spk{GetScriptForDestination(PKHash(pubkey))};

            AddSpentCoin(coins, tx_copy2, 10101, spk);

            const auto sig{SignInput(privkey, spk, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig << ToByteVector(pubkey);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2PKH input");
        }

        // Adding a 1-of-1 bare multisig input will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 61)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spent_script{CScript{} << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG};

            AddSpentCoin(coins, tx_copy2, 10102, spent_script);

            const auto sig{SignInput(privkey, spent_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << OP_0 << sig;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a bare 1-of-1 multisig input");
        }

        // Adding an input spending an empty Script but having a sigop in the scriptSig
        // will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 62)};
            const auto pubkey{privkey.GetPubKey()};
            const CScript spent_script;

            AddSpentCoin(coins, tx_copy2, 10103, spent_script);

            const auto signed_script{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};
            const auto sig{SignInput(privkey, signed_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig << ToByteVector(pubkey) << OP_CHECKSIG;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input with a CHECKSIG in the scriptSig");
        }

        // Adding an input spending a single CHECKSIG in a p2sh will make us exceed the
        // limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 63)};
            const auto pubkey{privkey.GetPubKey()};
            const auto redeem_script{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};
            const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

            AddSpentCoin(coins, tx_copy2, 10104, spk);

            const auto sig{SignInput(privkey, redeem_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH input with a single CHECKSIG");
        }

        // Adding an input spending a 1of1 multisig in a p2sh will make us exceed the
        // limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 64)};
            const auto pubkey{privkey.GetPubKey()};
            const auto redeem_script{CScript{} << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG};
            const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

            AddSpentCoin(coins, tx_copy2, 10105, spk);

            const auto sig{SignInput(privkey, redeem_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << OP_0 << sig << ToByteVector(redeem_script);
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH input with 1-of-1 CHECKMULTISIG");
        }

        // Adding an input spending an invalid Script but containing a CHECKSIG will
        // make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            const auto spent_script{CScript{} << OP_0 << OP_CHECKSIG};

            AddSpentCoin(coins, tx_copy2, 10106, spent_script);

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input spending an invalid bare Script containing a CHECKSIG");
        }

        // Adding an input spending an invalid p2sh but containing a CHECKMULTISIG
        // will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto redeem_script{CScript{} << OP_RETURN << OP_CHECKMULTISIG};
            const auto spk{GetScriptForDestination(ScriptHash(redeem_script))};

            AddSpentCoin(coins, tx_copy2, 10107, spk);
            tx_copy2.vin[idx].scriptSig << ToByteVector(redeem_script);

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input spending an invalid p2sh containing a CHECKMULTISIG");
        }

        // Adding an input spending a P2WPKH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 65)};
            const auto pubkey{privkey.GetPubKey()};
            const auto keyhash{WitnessV0KeyHash(pubkey)};
            const auto spk{GetScriptForDestination(keyhash)};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));

            const CAmount value{10108};
            AddSpentCoin(coins, tx_copy2, value, spk);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2WPKH input");
        }

        // Adding an input spending a P2WSH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 66)};
            const auto pubkey{privkey.GetPubKey()};
            const auto witscript{CScript{} << OP_1 << ToByteVector(pubkey) << OP_1 << OP_CHECKMULTISIG};
            const auto witprogram{WitnessV0ScriptHash(witscript)};
            const auto spk{GetScriptForDestination(witprogram)};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));
            Assert(keystore.AddCScript(witscript));

            const CAmount value{10109};
            AddSpentCoin(coins, tx_copy2, value, spk);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2WSH input");
        }

        // Adding an input spending a P2SH-P2WPKH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 67)};
            const auto pubkey{privkey.GetPubKey()};
            const auto keyhash{WitnessV0KeyHash(pubkey)};
            const auto witprogram{GetScriptForDestination(keyhash)};
            const auto spk{GetScriptForDestination(ScriptHash(witprogram))};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));
            Assert(keystore.AddCScript(witprogram));

            const CAmount value{10110};
            AddSpentCoin(coins, tx_copy2, value, spk);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH-P2WPKH input");
        }

        // Adding an input spending a P2SH-P2WSH will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FillableSigningProvider keystore;
            SignatureData dummy_sigdata;

            const auto privkey{GetKeyAt(xprv, 68)};
            const auto pubkey{privkey.GetPubKey()};
            const auto witscript{CScript{} << ToByteVector(pubkey) << OP_CHECKSIG};
            const auto witprogram{GetScriptForDestination(WitnessV0ScriptHash(witscript))};
            const auto spk{GetScriptForDestination(ScriptHash(witprogram))};
            Assert(keystore.AddKeyPubKey(privkey, pubkey));
            Assert(keystore.AddCScript(witscript));
            Assert(keystore.AddCScript(witprogram));

            const CAmount value{10111};
            AddSpentCoin(coins, tx_copy2, value, spk);

            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, dummy_sigdata));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a P2SH-P2WSH input");
        }

        // Adding an input spending a Taproot through the key path will not exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FlatSigningProvider keystore;
            SignatureData sigdata;

            const auto privkey{GetKeyAt(xprv, 69)};
            const auto pubkey{privkey.GetPubKey()};
            TaprootBuilder builder;
            builder.Finalize(XOnlyPubKey{pubkey});
            const auto spk{GetScriptForDestination(builder.GetOutput())};
            keystore.keys[pubkey.GetID()] = privkey;

            const CAmount value{10112};
            AddSpentCoin(coins, tx_copy2, value, spk);

            sigdata.tr_spenddata = builder.GetSpendData();
            auto spent_outputs{RecordSpent(coins, tx_copy2)};
            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, sigdata, spent_outputs));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value, std::move(spent_outputs)));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a Taproot key path spend input");
        }

        // Adding an input spending a Taproot through the script path will not exceed the limit. This is
        // true also for future tapleaf versions.
        {
            CMutableTransaction tx_copy2{tx_copy};
            FlatSigningProvider keystore;
            SignatureData sigdata;

            // Create a Taproot with one Tapscript leaf and one future-version leaf.
            TaprootBuilder builder;
            const auto privkey{GetKeyAt(xprv, 70)};
            const auto pubkey{privkey.GetPubKey()}, pubkey_future{GetKeyAt(xprv, 71).GetPubKey()};
            const auto leaf_script{CScript{} << ToByteVector(XOnlyPubKey{pubkey}) << OP_CHECKSIG};
            builder.Add(1, ToByteVector(leaf_script), TAPROOT_LEAF_TAPSCRIPT);
            const auto leaf_script_future{CScript{} << ToByteVector(XOnlyPubKey{pubkey_future}) << OP_CHECKSIG};
            const auto future_leaf_version{0xc2};
            builder.Add(1, ToByteVector(leaf_script_future), future_leaf_version);
            builder.Finalize(XOnlyPubKey::NUMS_H);
            const auto spk{GetScriptForDestination(builder.GetOutput())};

            // Record the coin paying to this Taproot.
            const CAmount value{10113};
            AddSpentCoin(coins, tx_copy2, value, spk);
            auto spent_outputs{RecordSpent(coins, tx_copy2)};

            // Spend through the Tapscript leaf.
            keystore.keys[pubkey.GetID()] = privkey;
            sigdata.tr_spenddata = builder.GetSpendData();
            Assert(SignSignature(keystore, spk, tx_copy2, idx, value, SIGHASH_ALL, sigdata, spent_outputs));
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value, spent_outputs));
            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a Taproot script path spend input");

            // Spend through the future-version leaf. The signing provider only knows Tapscript, and a leaf of an
            // unknown version succeeds unconditionally under consensus anyway, so build the witness by hand.
            const auto& control_blocks{sigdata.tr_spenddata.scripts.at({ToByteVector(leaf_script_future), future_leaf_version})};
            tx_copy2.vin[idx].scriptWitness.stack = {ToByteVector(leaf_script_future), *control_blocks.begin()};
            BOOST_REQUIRE(VerifyTxin(spk, tx_copy2, idx, value, std::move(spent_outputs)));
            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a Taproot script path spend input of a future leaf version");
        }

        // Adding an input spending a future witness program does not somehow make us exceed
        // the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 72)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spk{GetScriptForDestination(WitnessUnknown(4, ToByteVector(pubkey)))};

            AddSpentCoin(coins, tx_copy2, 10114, spk);
            tx_copy2.vin.back().scriptWitness.stack.push_back({0x42, 0x42});
            auto sig{"5da6d1157e4f2c45fa8441152f01e3e96898fd0c39326a47327d3bd024c4f6a2083e954ac803f8cb539aba8e31cdd9554a92309f47d489aa6a431ab262efa15a"_hex_v_u8};
            tx_copy2.vin.back().scriptWitness.stack.push_back(std::move(sig));
            tx_copy2.vin.back().scriptWitness.stack.push_back(ToByteVector(pubkey));

            CheckWithinBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + a future Segwit program input");
        }

        // Adding an input spending a bare Script with no sigop but with a sigop in the
        // scriptSig will make us exceed the limit.
        {
            CMutableTransaction tx_copy2{tx_copy};

            const auto privkey{GetKeyAt(xprv, 73)};
            const auto pubkey{privkey.GetPubKey()};
            const auto spent_script{CScript{} << OP_2 << OP_2 << OP_ADD << OP_4 << OP_EQUAL};

            AddSpentCoin(coins, tx_copy2, 10101, spent_script);
            tx_copy2.vin.back().scriptSig << OP_0 << ToByteVector(pubkey) << OP_CHECKSIG << OP_DROP;

            const auto sig{SignInput(privkey, spent_script, tx_copy2, idx)};
            tx_copy2.vin[idx].scriptSig << sig;
            BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy2, idx));

            CheckExceedsBIP54Limits(CTransaction(tx_copy2), coins, test_vectors, "Mixed input types reaching exactly 2500 BIP54-sigops + an input with one CHECKSIG in the scriptSig");
        }
    }

    // Demonstrate that CHECKMULTISIGs are also "accurately" counted in scriptSigs, using a
    // contrived example.
    // We spend an empty scriptPubKey with a scriptSig containing 166 unexecuted 15-pubkeys
    // CHECKMULTISIGVERIFY's, plus 10 CHECKSIGs. This reaches the BIP 54 limit but only exceeds
    // it once we add one more CHECKSIG, demonstrating that the 166 CMSV accounted for 15
    // sigops each and not 20.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx_copy{tx};

        // We spend a coin with an empty scriptPubkey.
        CScript spent_script{};
        AddSpentCoin(coins, tx_copy, 0, spent_script);

        // 166 * 15 = 2490 CMS sigops + 10 CHECKSIG sigops
        tx_copy.vin[0].scriptSig << OP_1 << OP_0 << OP_IF;
        for (int i{0}; i < 166; ++i) {
            tx_copy.vin[0].scriptSig << 15 << OP_CHECKMULTISIGVERIFY;
        }
        for (int i{0}; i < 10; ++i) {
            tx_copy.vin[0].scriptSig << OP_CHECKSIGVERIFY;
        }
        tx_copy.vin[0].scriptSig << OP_ENDIF;
        BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, 0));

        // We reach but don't exceed the limit yet.
        CheckWithinBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "CHECKMULTISIG's in scriptSig with 16 pubkeys or less are not rounded up to 20 sigops");

        // Add a single CHECKSIG.
        tx_copy.vin[0].scriptSig << OP_0 << OP_1 << OP_CHECKSIG << OP_DROP;
        BOOST_REQUIRE(VerifyTxin(spent_script, tx_copy, 0));

        // Now we do.
        CheckExceedsBIP54Limits(CTransaction(tx_copy), coins, test_vectors, "Enough sigops in scriptSig to exceed the BIP 54 limit (even if unexecuted)");
    }

    // Some historical transactions which would have exceeded the BIP54 sigops limit. For
    // a full list of such transactions, see this mailing list post:
    // https://gnusha.org/pi/bitcoindev/49dyqqkf5NqGlGdinp6SELIoxzE_ONh3UIj6-EB8S804Id5yROq-b1uGK8DUru66eIlWuhb5R3nhRRutwuYjemiuOOBS2FQ4KWDnEh0wLuA=@protonmail.com/
    struct HistoricalTx {
        const std::vector<CTxOut> spent_outputs;
        const CTransaction tx;
    };
    std::vector<HistoricalTx> historical_txs;

    // This is bea1c2b87fee95a203c5b5d9f3e5d0f472385c34cb5af02d0560aab973169683.
    {
        DataStream stream{"0100000001cce43bcf04bec97d9389e2e92705723a6f5869770dde262312fbcccdaba35c9702000000fd2703510000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004cc9afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafffffffff01905f0100000000001976a9140af76822b5d13fd23b7b9380184c66e9a543368388ac00000000"_hex};
        std::vector<CTxOut> spent_outputs = {
            CTxOut{100000, CScript{} << OP_HASH160 << "923fdf3ff05b994004e374c69c8a2196c9e79344"_hex << OP_EQUAL},
        };
        historical_txs.push_back({
            .spent_outputs = std::move(spent_outputs),
            .tx = CTransaction{deserialize, TX_WITH_WITNESS, stream},
        });
    }

    // This is 62fc8d091a7c597783981f00b889d72d24ad5e3e224dbe1c2a317aabef89217e.
    {
        DataStream stream{"0100000002321438a932139c6642dfd5c14f231aed8988e3519e116ed96600db86a3511c3001000000cd51004cc963afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf68ffffffff321438a932139c6642dfd5c14f231aed8988e3519e116ed96600db86a3511c3002000000cd51004cc963afafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafafaf68ffffffff01905f0100000000001976a9140af770d29305bc0cfee379cac4d3f932dfb343de88ac00000000"_hex};
        std::vector<CTxOut> spent_outputs = {
            CTxOut{50000, CScript{} << OP_HASH160 << "a9395da5c22d266bb90ea2cc695a92ffd68c5597"_hex << OP_EQUAL},
            CTxOut{50000, CScript{} << OP_HASH160 << "a9395da5c22d266bb90ea2cc695a92ffd68c5597"_hex << OP_EQUAL},
        };
        historical_txs.push_back({
            .spent_outputs = std::move(spent_outputs),
            .tx = CTransaction{deserialize, TX_WITH_WITNESS, stream},
        });
    }

    for (const auto& [spent_outputs, hist_tx]: historical_txs) {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};

        const CMutableTransaction mtx{hist_tx};
        for (size_t i{0}; i < hist_tx.vin.size(); ++i) {
            const auto& spent_txo{spent_outputs[i]};
            BOOST_REQUIRE(VerifyTxin(spent_txo.scriptPubKey, mtx, i));
            coins.AddCoin(hist_tx.vin[i].prevout, Coin(spent_txo, 0, false), false);
        }

        CheckExceedsBIP54Limits(CTransaction(hist_tx), coins, test_vectors, "Historical Bitcoin transaction " + hist_tx.GetHash().ToString());
    }

    // Now we move on to test some pathological transactions to demonstrates edge cases of the
    // sigop accounting functions. Those transactions would already be invalid anyways, but
    // this is useful to generate explicit test vectors.

    // CheckSigopsBIP54() does not validate Script. It will count also for (some) invalid Scripts.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        // Create an unspendable bare Script which exceeds 2500 sigops.
        CScript spk;
        spk.insert(spk.end(), MAX_TX_BIP54_SIGOPS + 1, OP_CHECKSIGVERIFY);

        AddSpentCoin(coins, tx2, 0, spk);

        // CheckSigopsBIP54 will return false despite the Script being invalid.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Invalid bare script with 2501 CHECKSIGs");
    }

    // CheckSigopsBIP54 uses GetSigOpCount, which will only count the number of sigops in
    // CHECKMULTISIG operations accurately if the CMS / CMSVERIFY opcode is directly preceded
    // by OP_1-OP_16 and will always count it for 20 sigops otherwise. The previous tests have
    // exercised OP_0 and various counts >16. This exercises having another opcode as the count.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        // The first CMS will have OP_INVALIDOPCODE as its "last op", and count for 20. All the
        // following 125 CMS will have the previous CMS as their "last op", and will similarly
        // count for 20.
        CScript spk;
        spk.insert(spk.end(), MAX_TX_BIP54_SIGOPS / MAX_PUBKEYS_PER_MULTISIG + 1, OP_CHECKMULTISIG);

        AddSpentCoin(coins, tx2, 0, spk);

        // CheckSigopsBIP54 will return false because there is 126 CMS that account for 20 each.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Invalid bare script with 126 CHECKMULTISIGs each accounted for 20 BIP54-sigops");
    }

    // Only OP_1 through OP_16 immediately before a CHECKMULTISIG select the accurate accounting. Other encodings
    // of small numbers, OP_1NEGATE and values above 16 all fall back to the conservative 20 sigops.
    const auto check_multisig_fallback{[&](CScript count_prefix, std::string comment) {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        // 20 sigops for the CHECKMULTISIG, plus enough CHECKSIGs to exceed the limit by one if the fallback applies.
        CScript spk{std::move(count_prefix)};
        spk << OP_CHECKMULTISIG;
        spk.insert(spk.end(), MAX_TX_BIP54_SIGOPS - MAX_PUBKEYS_PER_MULTISIG + 1, OP_CHECKSIG);
        AddSpentCoin(coins, tx2, 0, spk);

        CheckExceedsBIP54Limits(CTransaction{tx2}, coins, test_vectors, std::move(comment));
    }};
    check_multisig_fallback(CScript{} << OP_1NEGATE, "OP_1NEGATE before CHECKMULTISIG uses the 20-sigop fallback");
    for (const int64_t count : {1, 15, 16, 17, 20, 21, 100}) {
        check_multisig_fallback(CScript{} << CScriptNum{count}.getvch(), strprintf("Push-encoded %d before CHECKMULTISIG uses the 20-sigop fallback", count));
    }

    // Note this is also a limitation for legitimate Scripts, for instance if the arguments to
    // CMS are decided dynamically.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        const auto privkey{GetKeyAt(xprv, 100)};
        const auto pubkey{privkey.GetPubKey()};

        // Start with a dummy multisig which can be either a 1of1 or a 1of2. Then pad the Script
        // with 2481 dummy CHECKSIG's.
        auto spk{CScript{} << OP_IF << OP_1 << ToByteVector(pubkey) << OP_1 << OP_ELSE};
        spk << OP_1 << ToByteVector(pubkey) << ToByteVector(pubkey) << OP_2 << OP_ENDIF << OP_CHECKMULTISIG;
        spk.insert(spk.end(), MAX_TX_BIP54_SIGOPS - MAX_PUBKEYS_PER_MULTISIG + 1, OP_CHECKSIG);

        AddSpentCoin(coins, tx2, 0, spk);

        // CheckSigopsBIP54 will return false because the first CHECKMULTISIG counts for 20 sigops.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Invalid bare script with 1 CHECKMULTISIG accounted for 20 sigops + 2481 CHECKSIGs");
    }

    // In case of parsing error, CheckSigopsBIP54 will count sigops up to the point with incorrect encoding.
    // Here we have 2501 sigops and an invalid PUSHDATA1. This will fail the check.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        CScript spk;
        spk.insert(spk.end(), MAX_TX_BIP54_SIGOPS + 1, OP_CHECKSIG);
        spk.push_back(static_cast<uint8_t>(OP_PUSHDATA1));
        spk.push_back(0x01);

        AddSpentCoin(coins, tx2, 0, spk);

        // CheckSigopsBIP54 will return false because 2501 sigops were counted before encountering the error.
        CheckExceedsBIP54Limits(CTransaction(tx2), coins, test_vectors, "Bare Script with malformed PUSHDATA1 after counting 2501 BIP54-sigops");
    }

    // Now we have 2500 CHECKSIGs before the PUSHDATA2 parsing error, and one after. This will pass the check.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        CScript spk;
        spk.insert(spk.end(), MAX_TX_BIP54_SIGOPS, OP_CHECKSIG);
        spk.push_back(static_cast<uint8_t>(OP_PUSHDATA2));
        spk.push_back(0x42);
        spk.push_back(0x42);
        spk << OP_CHECKSIG;

        AddSpentCoin(coins, tx2, 0, spk);

        // CheckSigopsBIP54 will return true because 2500 sigops were counted before encountering the error.
        CheckWithinBIP54Limits(CTransaction(tx2), coins, test_vectors, "Bare Script with malformed PUSHDATA2 after counting 2500 BIP54-sigops");
    }

    // BIP16 counts zero P2SH sigops for a scriptSig which is not push-only, however many the redeemScript in its
    // final push contains. Such a transaction is invalid for other reasons, but the static accounting inherited
    // by BIP54 still stays within the limit.
    {
        CCoinsViewCache coins{&CoinsViewEmpty::Get()};
        CMutableTransaction tx2;

        CScript redeem_script;
        redeem_script.insert(redeem_script.end(), MAX_TX_BIP54_SIGOPS + 1, OP_CHECKSIG);
        AddSpentCoin(coins, tx2, 0, GetScriptForDestination(ScriptHash(redeem_script)));
        tx2.vin[0].scriptSig << OP_DUP << ToByteVector(redeem_script);

        CheckWithinBIP54Limits(CTransaction{tx2}, coins, test_vectors, "Non-push-only P2SH scriptSig whose redeemScript contains 2501 CHECKSIGs");
    }

    // Optionally dump test vectors as JSON. Uncomment UPDATE_JSON_TESTS at the top of this file to use.
#ifdef UPDATE_JSON_TESTS
    WriteJSONTestVectors(test_vectors, "bip54_sigops.json.gen");
#endif
}

/** BIP54 counts the sigops in the scripts spent by a transaction, not in the output scripts it creates. */
BOOST_FIXTURE_TEST_CASE(bip54_created_output_sigops, TestChain100Setup)
{
    CScript created_script;
    created_script.insert(created_script.end(), MAX_TX_BIP54_SIGOPS + 1, OP_CHECKSIG);
    const CMutableTransaction tx{CreateValidMempoolTransaction(m_coinbase_txns.front(), /*input_vout=*/0, /*input_height=*/1, coinbaseKey, created_script, /*output_amount=*/1 * COIN, /*submit=*/false)};
    BOOST_REQUIRE_EQUAL(tx.vout.front().scriptPubKey.GetSigOpCount(/*fAccurate=*/true), MAX_TX_BIP54_SIGOPS + 1);

    // The pre-existing block-wide limit does count the sigops in created outputs, but this block stays within it.
    const auto legacy_sigops{GetLegacySigOpCount(CTransaction{tx})};
    BOOST_REQUIRE_EQUAL(legacy_sigops, MAX_TX_BIP54_SIGOPS + 1);
    BOOST_REQUIRE_LE(static_cast<int64_t>(legacy_sigops) * WITNESS_SCALE_FACTOR, MAX_BLOCK_SIGOPS_COST);

    const CBlock block{CreateBlock({tx}, CScript{} << OP_TRUE)};
    ChainstateManager& chainman{*Assert(m_node.chainman)};
    LOCK(chainman.GetMutex());
    const auto state{TestBlockValidity(chainman.ActiveChainstate(), block, /*check_pow=*/true, /*check_merkle_root=*/true)};
    BOOST_CHECK_MESSAGE(state.IsValid(), state.ToString());
}

BOOST_AUTO_TEST_SUITE_END()
