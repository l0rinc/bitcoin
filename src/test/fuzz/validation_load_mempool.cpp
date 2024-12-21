// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mempool_persist.h>

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
#include <vector>

using node::DumpMempool;
using node::LoadMempool;

using node::MempoolPath;

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_validation_load_mempool()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(validation_load_mempool, .init = initialize_validation_load_mempool)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    size_t size = buffer.size();
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), size};
    assert(size == buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    assert(size == buffer.size());
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    assert(size == buffer.size());

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());

    auto& chainstate{static_cast<DummyChainState&>(g_setup->m_node.chainman->ActiveChainstate())};
    chainstate.SetMempool(&pool);

    assert(size == buffer.size());
    bool should_fail = buffer.size() < sizeof(uint64_t) + Obfuscation::SIZE_BYTES; // Inputs smaller than 9 bytes should fail
    auto fuzzed_fopen = [&](const fs::path&, const char*) {
        return fuzzed_file_provider.open();
    };
    try {
        (void)LoadMempool(pool, MempoolPath(g_setup->m_args), chainstate,
                          {
                              .mockable_fopen_function = fuzzed_fopen,
                          });
        assert(!should_fail);
    } catch (std::logic_error) {
        assert(should_fail);
    }
    pool.SetLoadTried(true);
    (void)DumpMempool(pool, MempoolPath(g_setup->m_args), fuzzed_fopen, true);
}
