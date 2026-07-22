// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/fs.h>
#include <util/strencodings.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/db.h>
#include <wallet/dump.h>
#include <wallet/migrate.h>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string_view>

using wallet::DatabaseOptions;
using wallet::DatabaseStatus;

namespace {
TestingSetup* g_setup;

void AssertDumpContracts(wallet::WalletDatabase& db, const fs::path& dump_path)
{
    auto batch{db.MakeBatch()};
    assert(batch);
    auto cursor{batch->GetNewCursor()};
    assert(cursor);

    size_t database_records{0};
    while (true) {
        DataStream key;
        DataStream value;
        switch (cursor->Next(key, value)) {
        case wallet::DatabaseCursor::Status::DONE:
            goto records_counted;
        case wallet::DatabaseCursor::Status::MORE:
            ++database_records;
            break;
        case wallet::DatabaseCursor::Status::FAIL:
            assert(false);
            return;
        }
    }

records_counted:
    std::ifstream dump_file{dump_path.std_path()};
    assert(dump_file.is_open());
    std::string line;
    assert(std::getline(dump_file, line));
    assert(line == "BITCOIN_CORE_WALLET_DUMP,1");
    assert(std::getline(dump_file, line));
    assert(line == "format,bdb");

    size_t dump_records{0};
    constexpr std::string_view checksum_prefix{"checksum,"};
    assert(std::getline(dump_file, line));
    while (!line.starts_with(checksum_prefix)) {
        assert(line.find(',') != std::string::npos);
        ++dump_records;
        assert(std::getline(dump_file, line));
    }
    assert(line.size() == checksum_prefix.size() + 64);
    assert(IsHex(std::string_view{line}.substr(checksum_prefix.size())));
    assert(!std::getline(dump_file, line));
    assert(dump_records == database_records);
}
} // namespace

void initialize_wallet_bdb_parser()
{
    static auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(wallet_bdb_parser, .init = initialize_wallet_bdb_parser)
{
    const auto wallet_path = g_setup->m_args.GetDataDirNet() / "fuzzed_wallet.dat";

    {
        AutoFile outfile{fsbridge::fopen(wallet_path, "wb")};
        outfile << std::span{buffer};
        assert(outfile.fclose() == 0);
    }

    const DatabaseOptions options{};
    DatabaseStatus status{DatabaseStatus::FAILED_LOAD};
    bilingual_str error;

    fs::path bdb_ro_dumpfile{g_setup->m_args.GetDataDirNet() / "fuzzed_dumpfile_bdb_ro.dump"};
    if (fs::exists(bdb_ro_dumpfile)) { // Writing into an existing dump file will throw an exception
        remove(bdb_ro_dumpfile);
    }
    g_setup->m_args.ForceSetArg("-dumpfile", fs::PathToString(bdb_ro_dumpfile));

    auto db{MakeBerkeleyRODatabase(wallet_path, options, status, error)};
    if (db) {
        assert(status == DatabaseStatus::SUCCESS);
        assert(DumpWallet(g_setup->m_args, *db, error));
        assert(error.original.empty());
        assert(fs::exists(bdb_ro_dumpfile));
        AssertDumpContracts(*db, bdb_ro_dumpfile);
    } else {
        assert(status == DatabaseStatus::FAILED_LOAD);
        assert(!error.original.empty());
        assert(!fs::exists(bdb_ro_dumpfile));
        if (error.original.starts_with("AutoFile::ignore: end of file") ||
            error.original.starts_with("AutoFile::read: end of file") ||
            error.original.starts_with("AutoFile::seek: ") ||
            error.original == "Not a BDB file" ||
            error.original == "Unexpected page type, should be 9 (BTree Metadata)" ||
            error.original == "Unexpected database flags, should only be 0x20 (subdatabases)" ||
            error.original == "Unexpected outer database root page type" ||
            error.original == "Unexpected number of entries in outer database root page" ||
            error.original == "Subdatabase page number has unexpected length" ||
            error.original == "Unknown record type in records page" ||
            error.original == "Unknown record type in internal page" ||
            error.original == "Unexpected page size" ||
            error.original == "Unexpected page type" ||
            error.original == "Page number mismatch" ||
            error.original == "Bad btree level" ||
            error.original == "Bad page size" ||
            error.original == "Meta page number mismatch" ||
            error.original == "Data record position not in page" ||
            error.original == "Internal record position not in page" ||
            error.original == "LSNs are not reset, this database is not completely flushed. Please reopen then close the database with a version that has BDB support" ||
            error.original == "Records page has odd number of records" ||
            error.original == "Bad overflow record page type" ||
            error.original == "BTree page has an unexpected level" ||
            error.original == "BTree Leaf page is not at level 1" ||
            error.original == "Subdatabase last page is greater than database last page" ||
            error.original == "Page number is greater than database last page" ||
            error.original == "Last page number could not fit in file" ||
            error.original == "Subdatabase has an unexpected name" ||
            error.original == "Unsupported BDB data file version number" ||
            error.original == "BDB builtin encryption is not supported") {
        } else {
            throw std::runtime_error(error.original);
        }
    }
}
