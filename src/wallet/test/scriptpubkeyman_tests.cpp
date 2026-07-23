// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <key_io.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <script/solver.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/wallet.h>
#include <wallet/test/util.h>

#include <boost/test/unit_test.hpp>
#include <sqlite3.h>

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

BOOST_AUTO_TEST_CASE(get_new_destination_write_failure)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_wallet_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    BOOST_CHECK(!spkm->GetNewDestination(OutputType::BECH32));
    LOCK(spkm->cs_desc_man);
    BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 0);
}

BOOST_AUTO_TEST_CASE(get_new_destination_topup_write_failure)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtKey(extkey) + "/*h)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);
    BOOST_REQUIRE(spkm->GetNewDestination(OutputType::BECH32));

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_wallet_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    BOOST_CHECK_EXCEPTION(spkm->GetNewDestination(OutputType::BECH32),
                          std::runtime_error, HasReason("TopUpWithDB: writing cache items failed"));
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
