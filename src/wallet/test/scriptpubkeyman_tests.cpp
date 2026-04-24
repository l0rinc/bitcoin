// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>
#include <key.h>
#include <key_io.h>
#include <sqlite3.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(scriptpubkeyman_tests, BasicTestingSetup)

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

BOOST_AUTO_TEST_CASE(descriptor_topup_noop_avoids_walletdb_writes)
{
    std::unique_ptr<interfaces::Chain>& chain = m_node.chain;

    CWallet keystore(chain.get(), "", CreateMockableWalletDatabase());
    auto* spk_man = CreateDescriptor(keystore, "wpkh(xprv9s21ZrQH143K2LE7W4Xf3jATf9jECxSb7wj91ZnmY4qEJrS66Qru9RFqq8xbkgT32ya6HqYJweFdJUEDf5Q6JFV7jMiUws7kQfe6Tv4RbfN/0h/0h/*h)", true);
    BOOST_REQUIRE(spk_man != nullptr);
    auto* db = dynamic_cast<MockableSQLiteDatabase*>(&keystore.GetDatabase());
    BOOST_REQUIRE(db != nullptr);

    const CTxDestination used_dest{*Assert(spk_man->GetNewDestination(OutputType::BECH32))};
    const CScript script{GetScriptForDestination(used_dest)};

    // The first match replenishes the look-ahead after getnewaddress() advanced next_index.
    (void)spk_man->MarkUnusedAddresses(script);
    const int changes_before{sqlite3_total_changes(db->m_db)};

    const auto affected = spk_man->MarkUnusedAddresses(script);
    const int changes_after{sqlite3_total_changes(db->m_db)};
    BOOST_CHECK(affected.empty());
    BOOST_CHECK_EQUAL(changes_after, changes_before);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
