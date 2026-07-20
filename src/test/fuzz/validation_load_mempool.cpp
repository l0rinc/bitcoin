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
#include <util/time.h>
#include <util/translation.h>
#include <validation.h>

#include <cstdint>
#include <map>
#include <set>
#include <vector>

using node::DumpMempool;
using node::LoadMempool;

using node::MempoolPath;

namespace {
const TestingSetup* g_setup;

struct MempoolState {
    std::set<Txid> txids;
    std::map<Txid, CAmount> fee_deltas;
    std::set<Txid> unbroadcast;
    uint64_t total_tx_size;
    CAmount total_fee;
    uint64_t sequence;
    bool load_tried;

    friend bool operator==(const MempoolState&, const MempoolState&) = default;
};

MempoolState CaptureMempoolState(const CTxMemPool& pool)
{
    MempoolState state{
        .txids = {},
        .fee_deltas = {},
        .unbroadcast = pool.GetUnbroadcastTxs(),
        .total_tx_size = 0,
        .total_fee = 0,
        .sequence = 0,
        .load_tried = pool.GetLoadTried(),
    };
    LOCK(pool.cs);
    for (const auto& entry : pool.mapTx) {
        state.txids.insert(entry.GetTx().GetHash());
    }
    state.fee_deltas = pool.mapDeltas;
    state.total_tx_size = pool.GetTotalTxSize();
    state.total_fee = pool.GetTotalFee();
    state.sequence = pool.GetSequence();
    return state;
}

void CheckMempoolState(const CTxMemPool& pool, Chainstate& chainstate)
{
    LOCK(::cs_main);
    pool.check(chainstate.CoinsTip(), chainstate.m_chain.Height() + 1);
    for (const auto& txid : pool.GetUnbroadcastTxs()) {
        Assert(pool.exists(txid));
    }
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
    pool.SetLoadTried(true);
    CheckMempoolState(pool, chainstate);
    const auto before_dump{CaptureMempoolState(pool)};
    (void)DumpMempool(pool, MempoolPath(g_setup->m_args), fuzzed_fopen, true);
    Assert(CaptureMempoolState(pool) == before_dump);
    CheckMempoolState(pool, chainstate);
}
