// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/txospenderindex.h>

#include <interfaces/chain.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/time.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace {
TestChain100Setup* g_setup{nullptr};
TxoSpenderIndex* g_index{nullptr};
COutPoint g_known_outpoint;
Txid g_known_spender;
uint256 g_known_block;

void CheckKnownSpender()
{
    const auto found{g_index->FindSpender(g_known_outpoint)};
    Assert(found.has_value());
    Assert(found->has_value());
    Assert((*found)->tx->GetHash() == g_known_spender);
    Assert((*found)->block_hash == g_known_block);
}

void initialize_txospenderindex()
{
    // Keep these owners as function-local statics so they are destroyed before
    // process-wide crypto and allocator state during normal fuzz shutdown.
    static const auto setup{MakeNoLogFileContext<TestChain100Setup>()};
    g_setup = setup.get();

    // Ensure the first generated coinbase is mature, then create one real
    // spend so both the index write and transaction lookup paths are covered.
    g_setup->mineBlocks(10);
    const auto coinbase{g_setup->m_coinbase_txns.front()};
    const CScript& coinbase_script{coinbase->vout.front().scriptPubKey};
    g_known_outpoint = COutPoint{coinbase->GetHash(), 0};
    const auto [spending, fee]{g_setup->CreateValidTransaction(
        {coinbase}, {g_known_outpoint}, /*input_height=*/1, {g_setup->coinbaseKey},
        {CTxOut{coinbase->GetValueOut(), coinbase_script}}, std::nullopt, std::nullopt)};
    (void)fee;
    const CBlock block{g_setup->CreateAndProcessBlock({spending}, coinbase_script)};
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();
    g_known_spender = spending.GetHash();
    g_known_block = block.GetHash();

    static const auto index{std::make_unique<TxoSpenderIndex>(interfaces::MakeChain(g_setup->m_node), 1 << 20, false, true)};
    g_index = index.get();
    Assert(g_index->Init());
    g_index->Sync();
    CheckKnownSpender();
}
} // namespace

FUZZ_TARGET(txospenderindex, .init = initialize_txospenderindex)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    // BaseIndex::Sync reads NodeClock for log pacing; pin mock time so
    // check_globals does not flag a system-time access at teardown.
    SetMockTime(1231006505);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    CheckKnownSpender();
    const size_t query_count{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 32)};
    for (size_t i{0}; i < query_count; ++i) {
        COutPoint outpoint{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)),
                           fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        if (fuzzed_data_provider.ConsumeBool()) outpoint = g_known_outpoint;

        const auto found{g_index->FindSpender(outpoint)};
        Assert(found.has_value());
        if (outpoint == g_known_outpoint) {
            Assert(found->has_value());
            Assert((*found)->tx->GetHash() == g_known_spender);
            Assert((*found)->block_hash == g_known_block);
        }
    }

    if (fuzzed_data_provider.ConsumeBool()) {
        g_index->Stop();
        Assert(g_index->Init());
        g_index->Sync();
        CheckKnownSpender();
    }
}
