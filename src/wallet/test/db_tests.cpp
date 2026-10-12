// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <crypto/common.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/translation.h>
#include <wallet/sqlite.h>
#include <wallet/migrate.h>
#include <wallet/test/util.h>
#include <wallet/walletutil.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

inline std::ostream& operator<<(std::ostream& os, const std::pair<const SerializeData, SerializeData>& kv)
{
    std::span key{kv.first}, value{kv.second};
    os << "(\"" << std::string_view{reinterpret_cast<const char*>(key.data()), key.size()} << "\", \""
       << std::string_view{reinterpret_cast<const char*>(value.data()), value.size()} << "\")";
    return os;
}

namespace wallet {

inline std::span<const std::byte> StringBytes(std::string_view str)
{
    return std::as_bytes(std::span{str});
}

static SerializeData StringData(std::string_view str)
{
    auto bytes = StringBytes(str);
    return SerializeData{bytes.begin(), bytes.end()};
}

static void CheckPrefix(DatabaseBatch& batch, std::span<const std::byte> prefix, MockableData expected)
{
    std::unique_ptr<DatabaseCursor> cursor = batch.GetNewPrefixCursor(prefix);
    MockableData actual;
    while (true) {
        DataStream key, value;
        DatabaseCursor::Status status = cursor->Next(key, value);
        if (status == DatabaseCursor::Status::DONE) break;
        BOOST_CHECK(status == DatabaseCursor::Status::MORE);
        BOOST_CHECK(
            actual.emplace(SerializeData(key.begin(), key.end()), SerializeData(value.begin(), value.end())).second);
    }
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_SUITE(db_tests, BasicTestingSetup)

static std::vector<std::unique_ptr<WalletDatabase>> TestDatabases(const fs::path& path_root)
{
    std::vector<std::unique_ptr<WalletDatabase>> dbs;
    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    // Unable to test BerkeleyRO since we cannot create a new BDB database to open
    dbs.emplace_back(MakeSQLiteDatabase(path_root / "sqlite", options, status, error));
    dbs.emplace_back(CreateMockableWalletDatabase());
    return dbs;
}

BOOST_AUTO_TEST_CASE(db_cursor_prefix_range_test)
{
    // Test each supported db
    for (const auto& database : TestDatabases(m_path_root)) {
        std::vector<std::string> prefixes = {"", "FIRST", "SECOND", "P\xfe\xff", "P\xff\x01", "\xff\xff"};

        std::unique_ptr<DatabaseBatch> handler = Assert(database)->MakeBatch();
        // Write elements to it
        for (unsigned int i = 0; i < 10; i++) {
            for (const auto& prefix : prefixes) {
                BOOST_CHECK(handler->Write(std::make_pair(prefix, i), i));
            }
        }

        // Now read all the items by prefix and verify that each element gets parsed correctly
        for (const auto& prefix : prefixes) {
            DataStream s_prefix;
            s_prefix << prefix;
            std::unique_ptr<DatabaseCursor> cursor = handler->GetNewPrefixCursor(s_prefix);
            DataStream key;
            DataStream value;
            for (int i = 0; i < 10; i++) {
                DatabaseCursor::Status status = cursor->Next(key, value);
                BOOST_CHECK_EQUAL(status, DatabaseCursor::Status::MORE);

                std::string key_back;
                unsigned int i_back;
                key >> key_back >> i_back;
                BOOST_CHECK_EQUAL(key_back, prefix);

                unsigned int value_back;
                value >> value_back;
                BOOST_CHECK_EQUAL(value_back, i_back);
            }

            // Let's now read it once more, it should return DONE
            BOOST_CHECK(cursor->Next(key, value) == DatabaseCursor::Status::DONE);
        }
        handler.reset();
        database->Close();
    }
}

// Lower level DatabaseBase::GetNewPrefixCursor test, to cover cases that aren't
// covered in the higher level test above. The higher level test uses
// serialized strings which are prefixed with string length, so it doesn't test
// truly empty prefixes or prefixes that begin with \xff
BOOST_AUTO_TEST_CASE(db_cursor_prefix_byte_test)
{
    const MockableData::value_type
        e{StringData(""), StringData("e")},
        p{StringData("prefix"), StringData("p")},
        ps{StringData("prefixsuffix"), StringData("ps")},
        f{StringData("\xff"), StringData("f")},
        fs{StringData("\xffsuffix"), StringData("fs")},
        ff{StringData("\xff\xff"), StringData("ff")},
        ffs{StringData("\xff\xffsuffix"), StringData("ffs")};
    for (const auto& database : TestDatabases(m_path_root)) {
        std::unique_ptr<DatabaseBatch> batch = database->MakeBatch();

        // Write elements to it if not berkeleyro
        for (const auto& [k, v] : {e, p, ps, f, fs, ff, ffs}) {
            batch->Write(std::span{k}, std::span{v});
        }

        CheckPrefix(*batch, StringBytes(""), {e, p, ps, f, fs, ff, ffs});
        CheckPrefix(*batch, StringBytes("prefix"), {p, ps});
        CheckPrefix(*batch, StringBytes("\xff"), {f, fs, ff, ffs});
        CheckPrefix(*batch, StringBytes("\xff\xff"), {ff, ffs});
        batch.reset();
        database->Close();
    }
}

BOOST_AUTO_TEST_CASE(berkeley_ro_cyclic_overflow_chain_rejected)
{
    constexpr size_t PAGE_SIZE{4096};
    constexpr uint32_t NUM_PAGES{5};
    constexpr uint32_t BTREE_MAGIC{0x00053162}, BTREE_VERSION{9}, BTREE_SUBDB{0x20};
    constexpr uint8_t BTREE_META{9}, BTREE_LEAF{5}, OVERFLOW_PAGE{7}, KEYDATA{1}, OVERFLOW_RECORD{3};
    std::vector<std::byte> file(PAGE_SIZE * NUM_PAGES);

    auto write8{[&](size_t offset, uint8_t value) { file[offset] = std::byte{value}; }};
    auto write16{[&](size_t offset, uint16_t value) { WriteLE16(file.data() + offset, value); }};
    auto write32{[&](size_t offset, uint32_t value) { WriteLE32(file.data() + offset, value); }};
    auto write_meta_page{[&](uint32_t page_num, uint32_t root) {
        size_t offset{page_num * PAGE_SIZE};
        write32(offset + 4, 1);
        write32(offset + 8, page_num);
        write32(offset + 12, BTREE_MAGIC);
        write32(offset + 16, BTREE_VERSION);
        write32(offset + 20, PAGE_SIZE);
        write8(offset + 25, BTREE_META);
        write32(offset + 32, NUM_PAGES - 1);
        write32(offset + 48, BTREE_SUBDB);
        write32(offset + 88, root);
    }};
    write_meta_page(/*page_num=*/0, /*root=*/1);
    write_meta_page(/*page_num=*/2, /*root=*/3);

    // Outer root leaf with ("main", BE32(2)) records.
    {
        size_t offset{PAGE_SIZE};
        write32(offset + 4, 1);
        write32(offset + 8, 1);
        write16(offset + 20, 2);
        write16(offset + 22, 4082);
        write8(offset + 24, 1);
        write8(offset + 25, BTREE_LEAF);
        write16(offset + 26, 4082);
        write16(offset + 28, 4089);
        offset += 4082;
        write16(offset, 4);
        write8(offset + 2, KEYDATA);
        for (size_t i{0}; i < 4; ++i)
            write8(offset + 3 + i, "main"[i]);
        write16(offset + 7, 4);
        write8(offset + 9, KEYDATA);
        write8(offset + 13, 2);
    }

    // Inner root leaf with a key and an overflow record pointing to page 4.
    {
        size_t offset{3 * PAGE_SIZE};
        write32(offset + 4, 1);
        write32(offset + 8, 3);
        write16(offset + 20, 2);
        write16(offset + 22, 4080);
        write8(offset + 24, 1);
        write8(offset + 25, BTREE_LEAF);
        write16(offset + 26, 4092);
        write16(offset + 28, 4080);
        offset += 4080;
        write8(offset + 2, OVERFLOW_RECORD);
        write32(offset + 4, 4);
        write32(offset + 8, 16);
        write16(offset + 12, 1);
        write8(offset + 14, KEYDATA);
        write8(offset + 15, 1);
    }

    // Empty overflow page with a self-cycle.
    {
        size_t offset{4 * PAGE_SIZE};
        write32(offset + 4, 1);
        write32(offset + 8, 4);
        write32(offset + 16, 4);
        write16(offset + 20, 1);
        write8(offset + 25, OVERFLOW_PAGE);
    }

    auto bdb_path{m_path_root / "cyclic_overflow_wallet.dat"};
    auto* handle{fsbridge::fopen(bdb_path, "wb")};
    BOOST_REQUIRE(handle);
    BOOST_REQUIRE_EQUAL(std::fwrite(file.data(), 1, file.size(), handle), file.size());
    BOOST_REQUIRE_EQUAL(std::fclose(handle), 0);

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    auto database{MakeBerkeleyRODatabase(bdb_path, options, status, error)};
    BOOST_CHECK(!database);
    BOOST_CHECK_EQUAL(status, DatabaseStatus::FAILED_LOAD);
    BOOST_CHECK_EQUAL(error.original, "Overflow record chain is cyclic or too long");
}

BOOST_AUTO_TEST_CASE(db_availability_after_write_error)
{
    // Ensures the database remains accessible without deadlocking after a write error.
    // To simulate the behavior, record overwrites are disallowed, and the test verifies
    // that the database remains active after failing to store an existing record.
    for (const auto& database : TestDatabases(m_path_root)) {
        // Write original record
        std::unique_ptr<DatabaseBatch> batch = database->MakeBatch();
        std::string key = "key";
        std::string value = "value";
        std::string value2 = "value_2";
        BOOST_CHECK(batch->Write(key, value));
        // Attempt to overwrite the record (expect failure)
        BOOST_CHECK(!batch->Write(key, value2, /*fOverwrite=*/false));
        // Successfully overwrite the record
        BOOST_CHECK(batch->Write(key, value2, /*fOverwrite=*/true));
        // Sanity-check; read and verify the overwritten value
        std::string read_value;
        BOOST_CHECK(batch->Read(key, read_value));
        BOOST_CHECK_EQUAL(read_value, value2);
    }
}

// Verify 'ErasePrefix' functionality using db keys similar to the ones used by the wallet.
// Keys are in the form of std::pair<TYPE, ENTRY_ID>
BOOST_AUTO_TEST_CASE(erase_prefix)
{
    const std::string key = "key";
    const std::string key2 = "key2";
    const std::string value = "value";
    const std::string value2 = "value_2";
    auto make_key = [](std::string type, std::string id) { return std::make_pair(type, id); };

    for (const auto& database : TestDatabases(m_path_root)) {
        if (dynamic_cast<BerkeleyRODatabase*>(database.get())) {
            // Skip this test if BerkeleyRO
            continue;
        }
        std::unique_ptr<DatabaseBatch> batch = database->MakeBatch();

        // Write two entries with the same key type prefix, a third one with a different prefix
        // and a fourth one with the type-id values inverted
        BOOST_CHECK(batch->Write(make_key(key, value), value));
        BOOST_CHECK(batch->Write(make_key(key, value2), value2));
        BOOST_CHECK(batch->Write(make_key(key2, value), value));
        BOOST_CHECK(batch->Write(make_key(value, key), value));

        // Erase the ones with the same prefix and verify result
        BOOST_CHECK(batch->TxnBegin());
        BOOST_CHECK(batch->ErasePrefix(DataStream() << key));
        BOOST_CHECK(batch->TxnCommit());

        BOOST_CHECK(!batch->Exists(make_key(key, value)));
        BOOST_CHECK(!batch->Exists(make_key(key, value2)));
        // Also verify that entries with a different prefix were not erased
        BOOST_CHECK(batch->Exists(make_key(key2, value)));
        BOOST_CHECK(batch->Exists(make_key(value, key)));
    }
}

// Test-only statement execution error
constexpr int TEST_SQLITE_ERROR = -999;

class DbExecBlocker : public SQliteExecHandler
{
private:
    SQliteExecHandler m_base_exec;
    std::set<std::string> m_blocked_statements;
public:
    DbExecBlocker(std::set<std::string> blocked_statements) : m_blocked_statements(blocked_statements) {}
    int Exec(SQLiteDatabase& database, const std::string& statement) override {
        if (m_blocked_statements.contains(statement)) return TEST_SQLITE_ERROR;
        return m_base_exec.Exec(database, statement);
    }
};

BOOST_AUTO_TEST_CASE(txn_close_failure_dangling_txn)
{
    // Verifies that there is no active dangling, to-be-reversed db txn
    // after the batch object that initiated it is destroyed.
    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    std::unique_ptr<SQLiteDatabase> database = MakeSQLiteDatabase(m_path_root / "sqlite", options, status, error);

    std::string key = "key";
    std::string value = "value";

    std::unique_ptr<SQLiteBatch> batch = std::make_unique<SQLiteBatch>(*database);
    BOOST_CHECK(batch->TxnBegin());
    BOOST_CHECK(batch->Write(key, value));
    // Set a handler to prevent txn abortion during destruction.
    // Mimicking a db statement execution failure.
    batch->SetExecHandler(std::make_unique<DbExecBlocker>(std::set<std::string>{"ROLLBACK TRANSACTION"}));
    // Destroy batch
    batch.reset();

    // Ensure there is no dangling, to-be-reversed db txn
    BOOST_CHECK(!database->HasActiveTxn());

    // And, just as a sanity check; verify that new batchs only write what they suppose to write
    // and nothing else.
    std::string key2 = "key2";
    std::unique_ptr<SQLiteBatch> batch2 = std::make_unique<SQLiteBatch>(*database);
    BOOST_CHECK(batch2->Write(key2, value));
    // The first key must not exist
    BOOST_CHECK(!batch2->Exists(key));
}

BOOST_AUTO_TEST_CASE(concurrent_txn_dont_interfere)
{
    std::string key = "key";
    std::string value = "value";
    std::string value2 = "value_2";

    DatabaseOptions options;
    DatabaseStatus status;
    bilingual_str error;
    const auto& database = MakeSQLiteDatabase(m_path_root / "sqlite", options, status, error);

    std::unique_ptr<DatabaseBatch> handler = Assert(database)->MakeBatch();

    // Verify concurrent db transactions does not interfere between each other.
    // Start db txn, write key and check the key does exist within the db txn.
    BOOST_CHECK(handler->TxnBegin());
    BOOST_CHECK(handler->Write(key, value));
    BOOST_CHECK(handler->Exists(key));

    // But, the same key, does not exist in another handler
    std::unique_ptr<DatabaseBatch> handler2 = Assert(database)->MakeBatch();
    BOOST_CHECK(handler2->Exists(key));

    // Attempt to commit the handler txn calling the handler2 methods.
    // Which, must not be possible.
    BOOST_CHECK(!handler2->TxnCommit());
    BOOST_CHECK(!handler2->TxnAbort());

    // Only the first handler can commit the changes.
    BOOST_CHECK(handler->TxnCommit());
    // And, once commit is completed, handler2 can read the record
    std::string read_value;
    BOOST_CHECK(handler2->Read(key, read_value));
    BOOST_CHECK_EQUAL(read_value, value);

    // Also, once txn is committed, single write statements are re-enabled.
    // Which means that handler2 can read the record changes directly.
    BOOST_CHECK(handler->Write(key, value2, /*fOverwrite=*/true));
    BOOST_CHECK(handler2->Read(key, read_value));
    BOOST_CHECK_EQUAL(read_value, value2);
}

BOOST_AUTO_TEST_CASE(in_memory_database_cannot_reopen)
{
    // Reopening an in-memory database would create a fresh empty connection,
    // silently losing all data. Open() must throw instead.
    InMemoryWalletDatabase database;
    database.Close();
    BOOST_CHECK_THROW(database.Open(), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
