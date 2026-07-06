// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <clientversion.h>
#include <streams.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(walletdb_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(walletdb_readkeyvalue)
{
    /**
     * When ReadKeyValue() reads from either a "key" or "wkey" it first reads the DataStream into a
     * CPrivKey or CWalletKey respectively and then reads a hash of the pubkey and privkey into a uint256.
     * Wallets from 0.8 or before do not store the pubkey/privkey hash, trying to read the hash from old
     * wallets throws an exception, for backwards compatibility this read is wrapped in a try block to
     * silently fail. The test here makes sure the type of exception thrown from DataStream::read()
     * matches the type we expect, otherwise we need to update the "key"/"wkey" exception type caught.
     */
    DataStream ssValue{};
    uint256 dummy;
    BOOST_CHECK_THROW(ssValue >> dummy, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(key_metadata_preserves_unsupported_flags)
{
    constexpr int version_with_flags{2};
    DataStream serialized{};
    serialized << version_with_flags;
    serialized << int64_t{123456789};
    serialized << uint8_t{0xa5};

    DataStream input{serialized};
    CKeyMetadata metadata;
    input >> metadata;

    BOOST_CHECK(input.empty());
    BOOST_CHECK_EQUAL(metadata.nVersion, version_with_flags);
    BOOST_CHECK_EQUAL(metadata.nCreateTime, int64_t{123456789});
    BOOST_CHECK_EQUAL(metadata.unsupported_key_flags, uint8_t{0xa5});

    DataStream roundtrip{};
    roundtrip << metadata;
    BOOST_CHECK_EQUAL(HexStr(serialized), HexStr(roundtrip));
}

BOOST_AUTO_TEST_CASE(walletdb_read_write_deadlock)
{
    // Exercises a db read write operation that shouldn't deadlock.
    for (const DatabaseFormat& db_format : DATABASE_FORMATS) {
        // Context setup
        DatabaseOptions options;
        options.require_format = db_format;
        DatabaseStatus status;
        bilingual_str error_string;
        std::unique_ptr<WalletDatabase> db = MakeDatabase(m_path_root / strprintf("wallet_%d_.dat", static_cast<int>(db_format)).c_str(), options, status, error_string);
        BOOST_REQUIRE(status == DatabaseStatus::SUCCESS);

        std::shared_ptr<CWallet> wallet(new CWallet(m_node.chain.get(), "", std::move(db)));
        wallet->m_keypool_size = 4;

        // Create legacy spkm
        LOCK(wallet->cs_wallet);
        auto legacy_spkm = wallet->GetOrCreateLegacyScriptPubKeyMan();
        BOOST_CHECK(!HasLegacyRecords(*wallet));
        BOOST_CHECK(legacy_spkm->SetupGeneration(true));
        BOOST_CHECK(HasLegacyRecords(*wallet));

        // Now delete all records, which performs a read write operation.
        WalletBatch batch(wallet->GetDatabase());
        BOOST_CHECK(wallet->GetLegacyScriptPubKeyMan()->DeleteRecordsWithDB(batch));
        BOOST_CHECK(!HasLegacyRecords(*wallet));
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
