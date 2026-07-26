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
#include <streams.h>

namespace wallet {

static bool DatabaseHasKey(MockableSQLiteDatabase& database, const DataStream& key)
{
    sqlite3_stmt* statement{nullptr};
    if (sqlite3_prepare_v2(database.m_db, "SELECT 1 FROM main WHERE key = ?", -1, &statement, nullptr) !=
        SQLITE_OK) {
        return false;
    }
    const bool bound{sqlite3_bind_blob(statement, 1, static_cast<const void*>(key.data()), key.size(), SQLITE_TRANSIENT) ==
                     SQLITE_OK};
    const bool present{bound && sqlite3_step(statement) == SQLITE_ROW};
    sqlite3_finalize(statement);
    return present;
}

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

BOOST_AUTO_TEST_CASE(encrypt_descriptor_write_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtKey(extkey) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);
    BOOST_REQUIRE(spkm->HavePrivateKeys());
    BOOST_CHECK(!spkm->HaveCryptedKeys());

    CKeyingMaterial master_key;
    master_key.resize(WALLET_CRYPTO_KEY_SIZE, 1);

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_wallet_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    WalletBatch batch(keystore.GetDatabase());
    BOOST_REQUIRE(batch.TxnBegin());
    BOOST_CHECK(!spkm->Encrypt(master_key, &batch));
    BOOST_REQUIRE(batch.TxnAbort());

    BOOST_CHECK(spkm->HavePrivateKeys());
    BOOST_CHECK(!spkm->HaveCryptedKeys());
    BOOST_CHECK(!spkm->CheckDecryptionKey(master_key));
}

BOOST_AUTO_TEST_CASE(encrypt_descriptor_erase_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtKey(extkey) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);
    BOOST_REQUIRE(spkm->HavePrivateKeys());

    CKeyingMaterial master_key;
    master_key.resize(WALLET_CRYPTO_KEY_SIZE, 1);

    const CPubKey pubkey{extkey.key.GetPubKey()};
    DataStream descriptor_key;
    descriptor_key << std::make_pair(DBKeys::WALLETDESCRIPTORKEY, std::make_pair(spkm->GetID(), pubkey));
    DataStream crypted_descriptor_key;
    crypted_descriptor_key << std::make_pair(DBKeys::WALLETDESCRIPTORCKEY, std::make_pair(spkm->GetID(), pubkey));

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE(DatabaseHasKey(database, descriptor_key));
    const std::string trigger{
        "CREATE TRIGGER fail_descriptor_key_erase BEFORE DELETE ON main WHEN lower(hex(OLD.key)) = '" +
        HexStr(std::span<const std::byte>{descriptor_key}) + "' BEGIN SELECT RAISE(ABORT, 'injected'); END;"};
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     trigger.c_str(),
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    WalletBatch batch(keystore.GetDatabase());
    BOOST_REQUIRE(batch.TxnBegin());
    const bool encrypted{spkm->Encrypt(master_key, &batch)};
    BOOST_CHECK(!encrypted);
    if (encrypted) {
        BOOST_REQUIRE(batch.TxnCommit());
    } else {
        BOOST_REQUIRE(batch.TxnAbort());
    }

    BOOST_CHECK(spkm->HavePrivateKeys());
    BOOST_CHECK(!spkm->HaveCryptedKeys());
    BOOST_CHECK(DatabaseHasKey(database, descriptor_key));
    BOOST_CHECK(!DatabaseHasKey(database, crypted_descriptor_key));
}

BOOST_AUTO_TEST_CASE(encrypt_wallet_descriptor_write_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtKey(extkey) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    const CPubKey pubkey{extkey.key.GetPubKey()};
    DataStream descriptor_key;
    descriptor_key << std::make_pair(DBKeys::WALLETDESCRIPTORKEY, std::make_pair(spkm->GetID(), pubkey));
    DataStream crypted_descriptor_key;
    crypted_descriptor_key << std::make_pair(DBKeys::WALLETDESCRIPTORCKEY, std::make_pair(spkm->GetID(), pubkey));

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE(DatabaseHasKey(database, descriptor_key));
    BOOST_CHECK(!DatabaseHasKey(database, crypted_descriptor_key));
    const std::string trigger{
        "CREATE TRIGGER fail_descriptor_key_write BEFORE INSERT ON main WHEN lower(hex(NEW.key)) = '" +
        HexStr(std::span<const std::byte>{crypted_descriptor_key}) + "' BEGIN SELECT RAISE(ABORT, 'injected'); END;"};
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db, trigger.c_str(), nullptr, nullptr, nullptr), SQLITE_OK);

    SecureString passphrase{"passphrase"};
    BOOST_CHECK(!keystore.EncryptWallet(passphrase));
    BOOST_CHECK(!keystore.HasEncryptionKeys());
    BOOST_CHECK(spkm->HavePrivateKeys());
    BOOST_CHECK(!spkm->HaveCryptedKeys());
    BOOST_CHECK(DatabaseHasKey(database, descriptor_key));
    BOOST_CHECK(!DatabaseHasKey(database, crypted_descriptor_key));
}

BOOST_AUTO_TEST_CASE(encrypt_wallet_master_key_write_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtKey(extkey) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_wallet_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    SecureString passphrase{"passphrase"};
    BOOST_CHECK(!keystore.EncryptWallet(passphrase));
    BOOST_CHECK(!keystore.HasEncryptionKeys());
    BOOST_CHECK(spkm->HavePrivateKeys());
    BOOST_CHECK(!spkm->HaveCryptedKeys());
}

BOOST_AUTO_TEST_CASE(get_new_destination_self_expanding_xpub)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK(spkm->GetWalletDescriptor().descriptor->CanSelfExpand());
        BOOST_CHECK(spkm->CanGetAddresses());
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 0);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().range_end, 1);
    }

    BOOST_REQUIRE(spkm->GetNewDestination(OutputType::BECH32));
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 1);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().range_end, 1);
        BOOST_CHECK(spkm->CanGetAddresses());
    }

    BOOST_REQUIRE(spkm->GetNewDestination(OutputType::BECH32));
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 2);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().range_end, 2);
    }
}

BOOST_AUTO_TEST_CASE(return_destination_write_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    int64_t reserved_index{-1};
    const auto reserved{spkm->GetReservedDestination(OutputType::BECH32, /*internal=*/false, reserved_index)};
    BOOST_REQUIRE(reserved);
    BOOST_CHECK_EQUAL(reserved_index, 0);
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 1);
    }

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_wallet_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    spkm->ReturnDestination(reserved_index, /*internal=*/false, *reserved);
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 1);
    }

    sqlite3_stmt* statement{nullptr};
    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT value FROM main WHERE length(key) = 49", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    SpanReader descriptor_reader{std::span<const std::byte>{static_cast<const std::byte*>(sqlite3_column_blob(statement, 0)), static_cast<size_t>(sqlite3_column_bytes(statement, 0))}};
    WalletDescriptor persisted_descriptor;
    descriptor_reader >> persisted_descriptor;
    BOOST_CHECK_EQUAL(persisted_descriptor.next_index, 1);
    sqlite3_finalize(statement);
}

BOOST_AUTO_TEST_CASE(mark_unused_persists_next_index)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    const std::string descriptor_string{"wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*)"};
    auto spkm = CreateDescriptor(keystore, descriptor_string, /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    BOOST_REQUIRE(spkm->GetNewDestination(OutputType::BECH32));
    BOOST_REQUIRE(spkm->TopUp(10));

    CScript index_one_script;
    {
        LOCK(spkm->cs_desc_man);
        const auto descriptor{spkm->GetWalletDescriptor()};
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts;
        BOOST_REQUIRE(descriptor.descriptor->ExpandFromCache(1, descriptor.cache, scripts, out_keys));
        BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
        index_one_script = scripts.front();
        BOOST_CHECK_EQUAL(descriptor.next_index, 1);
        BOOST_CHECK_EQUAL(descriptor.range_end, 11);
        BOOST_CHECK_EQUAL(spkm->GetEndRange(), 11);
    }

    const auto marked{spkm->MarkUnusedAddresses(index_one_script)};
    BOOST_CHECK_EQUAL(marked.size(), 1U);
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 2);
        BOOST_CHECK_EQUAL(spkm->GetEndRange(), 11);
    }

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    sqlite3_stmt* statement{nullptr};
    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT value FROM main WHERE length(key) = 49", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    SpanReader descriptor_reader{std::span<const std::byte>{static_cast<const std::byte*>(sqlite3_column_blob(statement, 0)), static_cast<size_t>(sqlite3_column_bytes(statement, 0))}};
    WalletDescriptor persisted_descriptor;
    descriptor_reader >> persisted_descriptor;
    BOOST_CHECK_EQUAL(persisted_descriptor.next_index, 2);
    BOOST_CHECK_EQUAL(sqlite3_step(statement), SQLITE_DONE);
    sqlite3_finalize(statement);
}

BOOST_AUTO_TEST_CASE(mark_unused_write_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    BOOST_REQUIRE(spkm->GetNewDestination(OutputType::BECH32));
    BOOST_REQUIRE(spkm->TopUp(10));

    CScript index_one_script;
    {
        LOCK(spkm->cs_desc_man);
        const auto descriptor{spkm->GetWalletDescriptor()};
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts;
        BOOST_REQUIRE(descriptor.descriptor->ExpandFromCache(1, descriptor.cache, scripts, out_keys));
        BOOST_REQUIRE_EQUAL(scripts.size(), 1U);
        index_one_script = scripts.front();
        BOOST_CHECK_EQUAL(descriptor.next_index, 1);
        BOOST_CHECK_EQUAL(descriptor.range_end, 11);
    }

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_descriptor_writes BEFORE INSERT ON main WHEN length(NEW.key) = 49 BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    BOOST_CHECK_EXCEPTION(spkm->MarkUnusedAddresses(index_one_script),
                          std::runtime_error, HasReason("TopUpWithDB: writing descriptor failed"));
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().next_index, 1);
    }

    sqlite3_stmt* statement{nullptr};
    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT value FROM main WHERE length(key) = 49", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    SpanReader descriptor_reader{std::span<const std::byte>{static_cast<const std::byte*>(sqlite3_column_blob(statement, 0)), static_cast<size_t>(sqlite3_column_bytes(statement, 0))}};
    WalletDescriptor persisted_descriptor;
    descriptor_reader >> persisted_descriptor;
    BOOST_CHECK_EQUAL(persisted_descriptor.next_index, 1);
    sqlite3_finalize(statement);
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
    // The serialized SQLite keys are 49 bytes for descriptor metadata and 62 bytes for cache entries.
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

BOOST_AUTO_TEST_CASE(topup_descriptor_write_failure)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    auto spkm = CreateDescriptor(keystore, "wpkh(" + EncodeExtKey(extkey) + "/*h)", /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_descriptor_writes BEFORE INSERT ON main WHEN length(NEW.key) = 49 BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    sqlite3_stmt* statement{nullptr};
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().range_end, 1);
        BOOST_CHECK_EQUAL(spkm->GetKeyPoolSize(), 1);
    }
    BOOST_CHECK_EXCEPTION(spkm->TopUp(2), std::runtime_error,
                          HasReason("TopUpWithDB: writing descriptor failed"));
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().range_end, 1);
        BOOST_CHECK_EQUAL(spkm->GetKeyPoolSize(), 1);
    }

    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT value FROM main WHERE length(key) = 49", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    SpanReader descriptor_reader{std::span<const std::byte>{static_cast<const std::byte*>(sqlite3_column_blob(statement, 0)), static_cast<size_t>(sqlite3_column_bytes(statement, 0))}};
    WalletDescriptor persisted_descriptor;
    descriptor_reader >> persisted_descriptor;
    BOOST_CHECK_EQUAL(persisted_descriptor.range_end, 1);
    sqlite3_finalize(statement);

    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT count(*) FROM main WHERE length(key) = 62", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    BOOST_CHECK_EQUAL(sqlite3_column_int(statement, 0), 1);
    sqlite3_finalize(statement);
}

BOOST_AUTO_TEST_CASE(update_descriptor_write_failure_preserves_state)
{
    CExtKey extkey;
    extkey.SetSeed(std::array<std::byte, 32>{});
    CWallet keystore(m_node.chain.get(), "", CreateMockableWalletDatabase());
    keystore.m_keypool_size = 1;
    const std::string descriptor_string{"wpkh(" + EncodeExtPubKey(extkey.Neuter()) + "/*)"};
    auto spkm = CreateDescriptor(keystore, descriptor_string, /*success=*/true);
    BOOST_REQUIRE(spkm != nullptr);

    FlatSigningProvider keys;
    std::string error;
    auto parsed_descs = Parse(descriptor_string, keys, error, /*require_checksum=*/false);
    BOOST_REQUIRE_EQUAL(parsed_descs.size(), 1U);
    WalletDescriptor replacement{std::move(parsed_descs.front()), 1, 0, 2, 0};

    auto& database = dynamic_cast<MockableSQLiteDatabase&>(keystore.GetDatabase());
    BOOST_REQUIRE_EQUAL(sqlite3_exec(database.m_db,
                                     "CREATE TRIGGER fail_descriptor_writes BEFORE INSERT ON main BEGIN SELECT RAISE(ABORT, 'injected'); END;",
                                     nullptr, nullptr, nullptr),
                        SQLITE_OK);

    BOOST_CHECK_EQUAL(spkm->GetScriptPubKeys().size(), 1U);
    BOOST_CHECK_EQUAL(spkm->GetEndRange(), 1);
    sqlite3_stmt* statement{nullptr};
    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT count(*) FROM main WHERE length(key) = 62", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    const int cache_rows_before{sqlite3_column_int(statement, 0)};
    sqlite3_finalize(statement);

    BOOST_CHECK_EXCEPTION(spkm->UpdateWalletDescriptor(replacement, keys),
                          std::runtime_error, HasReason("TopUpWithDB: writing cache items failed"));

    BOOST_CHECK_EQUAL(spkm->GetScriptPubKeys().size(), 1U);
    BOOST_CHECK_EQUAL(spkm->GetEndRange(), 1);
    {
        LOCK(spkm->cs_desc_man);
        BOOST_CHECK_EQUAL(spkm->GetWalletDescriptor().range_end, 1);
    }

    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT value FROM main WHERE length(key) = 49", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    SpanReader descriptor_reader{std::span<const std::byte>{static_cast<const std::byte*>(sqlite3_column_blob(statement, 0)), static_cast<size_t>(sqlite3_column_bytes(statement, 0))}};
    WalletDescriptor persisted_descriptor;
    descriptor_reader >> persisted_descriptor;
    BOOST_CHECK_EQUAL(persisted_descriptor.range_end, 1);
    sqlite3_finalize(statement);

    BOOST_REQUIRE_EQUAL(sqlite3_prepare_v2(database.m_db, "SELECT count(*) FROM main WHERE length(key) = 62", -1, &statement, nullptr), SQLITE_OK);
    BOOST_REQUIRE_EQUAL(sqlite3_step(statement), SQLITE_ROW);
    BOOST_CHECK_EQUAL(sqlite3_column_int(statement, 0), cache_rows_before);
    sqlite3_finalize(statement);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
