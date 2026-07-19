// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <validation.h>

#include <cassert>
#include <cstddef>
#include <memory>

namespace {
ChainTestingSetup* g_testing_setup;

void initialize_chainstate_lifecycle()
{
    static const auto setup{MakeNoLogFileContext<ChainTestingSetup>()};
    g_testing_setup = setup.get();
}
} // namespace

// Exercise the ChainstateManager add/delete/reindex lifecycle with and
// without a mempool, mirroring the kernel API (no mempool) and the reindex
// wipe path in node::LoadChainstate(). The ChainstateManager previously
// crashed deleting a snapshot chainstate while holding a null mempool.
FUZZ_TARGET(chainstate_lifecycle, .init = initialize_chainstate_lifecycle)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fdp{buffer.data(), buffer.size()};
    ChainstateManager& chainman{*Assert(g_testing_setup->m_node.chainman)};
    CTxMemPool* const mempool{g_testing_setup->m_node.mempool.get()};

    const bool with_mempool{fdp.ConsumeBool()};
    const size_t num_cycles{fdp.ConsumeIntegralInRange<size_t>(1, 4)};

    LOCK(::cs_main);
    chainman.ResetChainstates();
    Chainstate& ibd{chainman.InitializeChainstate(with_mempool ? mempool : nullptr)};

    for (size_t i{0}; i < num_cycles && fdp.remaining_bytes() > 0; ++i) {
        const uint256 snapshot_hash{uint256::ONE};
        Chainstate& snapshot{chainman.AddChainstate(std::make_unique<Chainstate>(nullptr, chainman.m_blockman, chainman, snapshot_hash))};

        // Leave a real coins db on disk, then release the views, mirroring a
        // snapshot chainstate dir from a previous process (deleted on reindex).
        snapshot.InitCoinsDB(8_MiB, /*in_memory=*/false, /*should_wipe=*/true);
        snapshot.ResetCoinsViews();

        if (fdp.ConsumeBool()) {
            // Mirror node::LoadChainstate(): reset the IBD chainstate target
            // from the snapshot block back to the network tip before deleting.
            ibd.SetTargetBlock(nullptr);
            const bool deleted{chainman.DeleteChainstate(snapshot)};
            assert(deleted);
        }

        // Invariant: at most one chainstate holds the mempool.
        size_t mempool_holders{0};
        for (const auto& cs : chainman.m_chainstates) {
            if (cs->GetMempool()) ++mempool_holders;
        }
        assert(mempool_holders <= 1);

        // If the snapshot was kept, the next AddChainstate would not be valid
        // (its previous chainstate is not a validated, targetless one).
        if (chainman.m_chainstates.size() > 1) break;
    }
}
