// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_persist.h>
#include <test/util/time.h>

#include <node/mempool_args.h>
#include <node/mempool_persist_args.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using node::DumpMempool;
using node::LoadMempool;

using node::MempoolPath;

namespace {
const TestingSetup* g_setup;

void AssertMempoolPersistContracts(const CTxMemPool& pool, Chainstate& chainstate)
{
    WITH_LOCK(::cs_main, pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1));
    for (const auto& txid : pool.GetUnbroadcastTxs()) {
        Assert(pool.exists(txid));
    }
}

struct MempoolEntrySnapshot {
    Txid txid;
    Wtxid wtxid;
    std::chrono::seconds time;
    CAmount fee;
    int32_t vsize;
    int64_t fee_delta;
};

struct PrioritisationSnapshot {
    Txid txid;
    bool in_mempool;
    CAmount delta;
    std::optional<CAmount> modified_fee;
};

std::vector<MempoolEntrySnapshot> SnapshotEntries(const CTxMemPool& pool)
{
    std::vector<MempoolEntrySnapshot> entries;
    for (const auto& info : pool.infoAll()) {
        entries.push_back({
            info.tx->GetHash(),
            info.tx->GetWitnessHash(),
            info.m_time,
            info.fee,
            info.vsize,
            info.nFeeDelta,
        });
    }
    std::ranges::sort(entries, {}, &MempoolEntrySnapshot::txid);
    return entries;
}

std::vector<PrioritisationSnapshot> SnapshotPrioritisations(const CTxMemPool& pool)
{
    std::vector<PrioritisationSnapshot> prioritisations;
    for (const auto& delta : pool.GetPrioritisedTransactions()) {
        prioritisations.push_back({
            delta.txid,
            delta.in_mempool,
            delta.delta,
            delta.modified_fee,
        });
    }
    std::ranges::sort(prioritisations, {}, &PrioritisationSnapshot::txid);
    return prioritisations;
}

void AssertSameMempoolSnapshot(const CTxMemPool& expected, const CTxMemPool& actual)
{
    const auto expected_entries{SnapshotEntries(expected)};
    const auto actual_entries{SnapshotEntries(actual)};
    Assert(expected_entries.size() == actual_entries.size());
    for (size_t i{0}; i < expected_entries.size(); ++i) {
        Assert(expected_entries[i].txid == actual_entries[i].txid);
        Assert(expected_entries[i].wtxid == actual_entries[i].wtxid);
        Assert(expected_entries[i].time == actual_entries[i].time);
        Assert(expected_entries[i].fee == actual_entries[i].fee);
        Assert(expected_entries[i].vsize == actual_entries[i].vsize);
        Assert(expected_entries[i].fee_delta == actual_entries[i].fee_delta);
    }

    const auto expected_prioritisations{SnapshotPrioritisations(expected)};
    const auto actual_prioritisations{SnapshotPrioritisations(actual)};
    Assert(expected_prioritisations.size() == actual_prioritisations.size());
    for (size_t i{0}; i < expected_prioritisations.size(); ++i) {
        Assert(expected_prioritisations[i].txid == actual_prioritisations[i].txid);
        Assert(expected_prioritisations[i].in_mempool == actual_prioritisations[i].in_mempool);
        Assert(expected_prioritisations[i].delta == actual_prioritisations[i].delta);
        Assert(expected_prioritisations[i].modified_fee == actual_prioritisations[i].modified_fee);
    }

    Assert(expected.GetUnbroadcastTxs() == actual.GetUnbroadcastTxs());
}

void AssertMempoolSnapshotMatchesImportOptions(const CTxMemPool& expected,
                                               const CTxMemPool& actual,
                                               bool use_current_time,
                                               bool apply_fee_delta_priority,
                                               bool apply_unbroadcast_set)
{
    const auto expected_entries{SnapshotEntries(expected)};
    const auto actual_entries{SnapshotEntries(actual)};
    Assert(expected_entries.size() == actual_entries.size());
    const auto current_time{std::chrono::seconds{TicksSinceEpoch<std::chrono::seconds>(NodeClock::now())}};
    for (size_t i{0}; i < expected_entries.size(); ++i) {
        Assert(expected_entries[i].txid == actual_entries[i].txid);
        Assert(expected_entries[i].wtxid == actual_entries[i].wtxid);
        Assert(actual_entries[i].time == (use_current_time ? current_time : expected_entries[i].time));
        Assert(expected_entries[i].fee == actual_entries[i].fee);
        Assert(expected_entries[i].vsize == actual_entries[i].vsize);
        Assert(actual_entries[i].fee_delta == (apply_fee_delta_priority ? expected_entries[i].fee_delta : 0));
    }

    if (apply_fee_delta_priority) {
        const auto expected_prioritisations{SnapshotPrioritisations(expected)};
        const auto actual_prioritisations{SnapshotPrioritisations(actual)};
        Assert(expected_prioritisations.size() == actual_prioritisations.size());
        for (size_t i{0}; i < expected_prioritisations.size(); ++i) {
            Assert(expected_prioritisations[i].txid == actual_prioritisations[i].txid);
            Assert(expected_prioritisations[i].in_mempool == actual_prioritisations[i].in_mempool);
            Assert(expected_prioritisations[i].delta == actual_prioritisations[i].delta);
            Assert(expected_prioritisations[i].modified_fee == actual_prioritisations[i].modified_fee);
        }
    } else {
        Assert(SnapshotPrioritisations(actual).empty());
    }

    if (apply_unbroadcast_set) {
        Assert(expected.GetUnbroadcastTxs() == actual.GetUnbroadcastTxs());
    } else {
        Assert(actual.GetUnbroadcastTxs().empty());
    }
}

void WriteFileBytes(const fs::path& path, const std::vector<std::byte>& bytes)
{
    AutoFile file{fsbridge::fopen(path, "wb")};
    Assert(!file.IsNull());
    file.write({bytes.data(), bytes.size()});
    Assert(file.fclose() == 0);
}

std::vector<std::byte> ReadFileBytes(const fs::path& path)
{
    AutoFile file{fsbridge::fopen(path, "rb")};
    Assert(!file.IsNull());
    std::vector<std::byte> bytes(file.size());
    file.read({bytes.data(), bytes.size()});
    return bytes;
}

uint64_t ReadDumpVersion(const fs::path& path)
{
    AutoFile file{fsbridge::fopen(path, "rb")};
    Assert(!file.IsNull());
    uint64_t version;
    file >> version;
    return version;
}

void AssertFailedDumpPreservesFile(const CTxMemPool& pool, const fs::path& path, const std::vector<std::byte>& existing_bytes, fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen)
{
    fs::remove_all(path);
    fs::remove_all(path + ".new");
    WriteFileBytes(path, existing_bytes);
    Assert(!DumpMempool(pool, path, std::move(mockable_fopen_function)));
    Assert(ReadFileBytes(path) == existing_bytes);
}
} // namespace

void initialize_validation_load_mempool()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(validation_load_mempool, .init = initialize_validation_load_mempool)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());

    auto& chainstate{static_cast<DummyChainState&>(g_setup->m_node.chainman->ActiveChainstate())};
    chainstate.SetMempool(&pool);

    auto fuzzed_fopen = [&](const fs::path&, const char*) {
        return fuzzed_file_provider.open();
    };
    (void)LoadMempool(pool, MempoolPath(g_setup->m_args), chainstate,
                      {
                          .mockable_fopen_function = fuzzed_fopen,
                      });
    AssertMempoolPersistContracts(pool, chainstate);
    if (fuzzed_data_provider.ConsumeBool()) {
        const Txid txid{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};
        const bool in_mempool{pool.exists(txid)};
        pool.AddUnbroadcastTx(txid);
        if (!in_mempool) {
            Assert(!pool.GetUnbroadcastTxs().contains(txid));
        }
        AssertMempoolPersistContracts(pool, chainstate);
    }
    pool.SetLoadTried(true);
    (void)DumpMempool(pool, MempoolPath(g_setup->m_args), fuzzed_fopen, true);
    AssertMempoolPersistContracts(pool, chainstate);

    const fs::path roundtrip_path{g_setup->m_args.GetDataDirBase() / "validation_load_mempool_roundtrip.dat"};
    fs::remove(roundtrip_path);
    fs::remove(roundtrip_path + ".new");
    Assert(DumpMempool(pool, roundtrip_path));
    Assert(ReadDumpVersion(roundtrip_path) == 2);

    const fs::path v1_roundtrip_path{g_setup->m_args.GetDataDirBase() / "validation_load_mempool_roundtrip_v1.dat"};
    fs::remove(v1_roundtrip_path);
    fs::remove(v1_roundtrip_path + ".new");
    auto v1_options{MemPoolOptionsForTest(g_setup->m_node)};
    v1_options.persist_v1_dat = true;
    bilingual_str v1_error;
    CTxMemPool v1_pool{v1_options, v1_error};
    Assert(v1_error.empty());
    chainstate.SetMempool(&v1_pool);
    Assert(LoadMempool(v1_pool, roundtrip_path, chainstate, {}));
    AssertMempoolPersistContracts(v1_pool, chainstate);
    AssertSameMempoolSnapshot(pool, v1_pool);
    Assert(DumpMempool(v1_pool, v1_roundtrip_path));
    Assert(ReadDumpVersion(v1_roundtrip_path) == 1);

    bilingual_str v1_reload_error;
    CTxMemPool v1_reload_pool{MemPoolOptionsForTest(g_setup->m_node), v1_reload_error};
    Assert(v1_reload_error.empty());
    chainstate.SetMempool(&v1_reload_pool);
    Assert(LoadMempool(v1_reload_pool, v1_roundtrip_path, chainstate, {}));
    AssertMempoolPersistContracts(v1_reload_pool, chainstate);
    AssertSameMempoolSnapshot(pool, v1_reload_pool);

    bilingual_str roundtrip_error;
    CTxMemPool roundtrip_pool{MemPoolOptionsForTest(g_setup->m_node), roundtrip_error};
    Assert(roundtrip_error.empty());
    chainstate.SetMempool(&roundtrip_pool);
    Assert(LoadMempool(roundtrip_pool, roundtrip_path, chainstate, {}));
    AssertMempoolPersistContracts(roundtrip_pool, chainstate);
    AssertSameMempoolSnapshot(pool, roundtrip_pool);
    Assert(LoadMempool(roundtrip_pool, roundtrip_path, chainstate,
                       {
                           .apply_fee_delta_priority = false,
                           .apply_unbroadcast_set = false,
                       }));
    AssertMempoolPersistContracts(roundtrip_pool, chainstate);
    AssertSameMempoolSnapshot(pool, roundtrip_pool);

    const bool use_current_time{fuzzed_data_provider.ConsumeBool()};
    const bool apply_fee_delta_priority{fuzzed_data_provider.ConsumeBool()};
    const bool apply_unbroadcast_set{fuzzed_data_provider.ConsumeBool()};
    bilingual_str options_error;
    CTxMemPool options_pool{MemPoolOptionsForTest(g_setup->m_node), options_error};
    Assert(options_error.empty());
    chainstate.SetMempool(&options_pool);
    Assert(LoadMempool(options_pool, roundtrip_path, chainstate,
                       {
                           .use_current_time = use_current_time,
                           .apply_fee_delta_priority = apply_fee_delta_priority,
                           .apply_unbroadcast_set = apply_unbroadcast_set,
                       }));
    AssertMempoolPersistContracts(options_pool, chainstate);
    AssertMempoolSnapshotMatchesImportOptions(pool, options_pool, use_current_time, apply_fee_delta_priority, apply_unbroadcast_set);
    chainstate.SetMempool(&pool);

    const std::vector<std::byte> existing_dump{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 128)};
    const fs::path atomic_path{g_setup->m_args.GetDataDirBase() / "validation_load_mempool_atomic.dat"};
    fs::remove_all(atomic_path);
    fs::remove_all(atomic_path + ".new");
    WriteFileBytes(atomic_path, existing_dump);
    Assert(fs::create_directory(atomic_path + ".new"));
    Assert(!DumpMempool(pool, atomic_path));
    Assert(ReadFileBytes(atomic_path) == existing_dump);
    fs::remove_all(atomic_path + ".new");

    const fs::path redirected_path{g_setup->m_args.GetDataDirBase() / "validation_load_mempool_redirected.dat"};
    fs::remove_all(redirected_path);
    AssertFailedDumpPreservesFile(pool, atomic_path, existing_dump, [&](const fs::path&, const char* mode) {
        return fsbridge::fopen(redirected_path, mode);
    });
    fs::remove_all(redirected_path);

    fs::remove(roundtrip_path);
    fs::remove(roundtrip_path + ".new");
    fs::remove(v1_roundtrip_path);
    fs::remove(v1_roundtrip_path + ".new");
    fs::remove_all(atomic_path);
    fs::remove_all(atomic_path + ".new");
}
