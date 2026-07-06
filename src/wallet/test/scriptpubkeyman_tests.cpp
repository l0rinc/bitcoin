// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <key.h>
#include <key_io.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

// Test LegacyScriptPubKeyMan::CanProvide behavior, making sure it returns true
// for recognized scripts even when keys may not be available for signing.
BOOST_AUTO_TEST_CASE(CanProvide)
{
    // Set up wallet and keyman variables.
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    LegacyScriptPubKeyMan& keyman = *wallet.GetOrCreateLegacyScriptPubKeyMan();

    // Make a 1 of 2 multisig script
    std::vector<CKey> keys(2);
    std::vector<CPubKey> pubkeys;
    for (CKey& key : keys) {
        key.MakeNewKey(true);
        pubkeys.emplace_back(key.GetPubKey());
    }
    CScript multisig_script = GetScriptForMultisig(1, pubkeys);
    CScript p2sh_script = GetScriptForDestination(ScriptHash(multisig_script));
    SignatureData data;

    // Verify the p2sh(multisig) script is not recognized until the multisig
    // script is added to the keystore to make it solvable
    BOOST_CHECK(!keyman.CanProvide(p2sh_script, data));
    keyman.AddCScript(multisig_script);
    BOOST_CHECK(keyman.CanProvide(p2sh_script, data));
}

BOOST_AUTO_TEST_CASE(DescriptorScriptPubKeyManTests)
{
    std::unique_ptr<interfaces::Chain>& chain = m_node.chain;

    CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
    auto key_scriptpath = GenerateRandomKey();

    // Verify that a SigningProvider for a pubkey is only returned if its corresponding private key is available
    auto key_internal = GenerateRandomKey();
    std::string desc_str = "tr(" + EncodeSecret(key_internal) + ",pk(" + HexStr(key_scriptpath.GetPubKey()) + "))";
    auto spk_man1 = CreateDescriptor(keystore, desc_str, true);
    BOOST_CHECK(spk_man1 != nullptr);
    auto signprov_keypath_spendable = spk_man1->GetSigningProvider(key_internal.GetPubKey());
    BOOST_CHECK(signprov_keypath_spendable != nullptr);

    desc_str = "tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",pk(" + HexStr(key_scriptpath.GetPubKey()) + "))";
    auto spk_man2 = CreateDescriptor(keystore, desc_str, true);
    BOOST_CHECK(spk_man2 != nullptr);
    auto signprov_keypath_nums_h = spk_man2->GetSigningProvider(XOnlyPubKey::NUMS_H.GetEvenCorrespondingCPubKey());
    BOOST_CHECK(signprov_keypath_nums_h == nullptr);
}

BOOST_AUTO_TEST_CASE(desc_spkm_topup_fail)
{
    // Attempting to construct a DescriptorSPKM that cannot be topped up (hardened derivation without private keys)
    // should throw even though it is valid and can be parsed
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    BOOST_CHECK_EXCEPTION(
        CreateDescriptor(keystore, "wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*h)", /*success=*/true),
        std::runtime_error, HasReason("Could not top up scriptPubKeys"));
}

BOOST_AUTO_TEST_CASE(Descriptor_IsKeyActive)
{
    CWallet wallet(m_node.chain.get(), "", CreateMockableWalletDatabase());
    {
        LOCK(wallet.cs_wallet);
        wallet.SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet.m_keypool_size = 10;
        wallet.SetupDescriptorScriptPubKeyMans();
    }
    auto* spkm{dynamic_cast<DescriptorScriptPubKeyMan*>(wallet.GetScriptPubKeyMan(OutputType::BECH32, /*internal=*/false))};
    BOOST_REQUIRE(spkm != nullptr);

    auto scripts1{spkm->GetScriptPubKeys()};
    BOOST_CHECK_EQUAL(scripts1.size(), 10);
    for (const CScript& script : scripts1) {
        BOOST_CHECK(spkm->IsKeyActive(script));
    }

    auto dest1{spkm->GetNewDestination(OutputType::BECH32)};
    BOOST_REQUIRE(dest1);
    CScript script{GetScriptForDestination(*dest1)};
    BOOST_CHECK(spkm->IsKeyActive(script));

    auto scripts2{spkm->GetScriptPubKeys()};
    BOOST_CHECK_EQUAL(scripts2.size(), 10);

    {
        LOCK(spkm->cs_desc_man);
        WalletDescriptor descriptor{spkm->GetWalletDescriptor()};
        FlatSigningProvider provider;
        std::vector<CScript> scripts3;
        BOOST_REQUIRE(descriptor.descriptor->ExpandFromCache(/*pos=*/5, descriptor.cache, scripts3, provider));

        BOOST_CHECK_EQUAL(scripts3.size(), 1);
        spkm->MarkUnusedAddresses(scripts3.front());
    }

    auto scripts4{spkm->GetScriptPubKeys()};
    BOOST_CHECK_EQUAL(scripts4.size(), 16);
    for (const CScript& script : scripts4) {
        BOOST_CHECK(spkm->IsKeyActive(script));
    }

    {
        LOCK(wallet.cs_wallet);
        wallet.SetupDescriptorScriptPubKeyMans();
    }

    for (const CScript& script : scripts4) {
        BOOST_CHECK(spkm->IsKeyActive(script));

        CTxDestination dest;
        BOOST_REQUIRE(ExtractDestination(script, dest));
        BOOST_CHECK(!wallet.IsDestinationActive(dest));
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
