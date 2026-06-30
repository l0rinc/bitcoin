// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/validation.h>
#include <uint256.h>
#include <validation.h>
#include <validationinterface.h>
#include <versionbits.h>

#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
TestingSetup* g_setup;

struct DagBlock {
    std::shared_ptr<const CBlock> block;
    bool valid;
};

std::vector<DagBlock> g_blocks;

bool IsAncestorOrDescendant(const CBlockIndex& a, const CBlockIndex& b)
{
    return a.GetAncestor(b.nHeight) == &b || b.GetAncestor(a.nHeight) == &a;
}

enum class ChainEventType {
    CONNECTED,
    DISCONNECTED,
    UPDATED_TIP,
    FLUSHED,
};

struct ChainEvent {
    ChainEventType type;
    uint256 hash;
    uint256 prev_hash;
    int height;
    int fork_height{-1};
};

struct TipEvent {
    uint256 hash;
    int height;
};

struct ValidationEventRecorder final : public CValidationInterface {
    std::vector<ChainEvent> m_events;
    std::vector<TipEvent> m_active_tip_events;
    uint256 m_expected_tip;
    int m_expected_height;
    std::optional<TipEvent> m_update_start_tip;

    explicit ValidationEventRecorder(const CBlockIndex& tip)
        : m_expected_tip{tip.GetBlockHash()}, m_expected_height{tip.nHeight}
    {
    }
    ~ValidationEventRecorder() = default;

    void BlockConnected(const kernel::ChainstateRole& role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        assert(block);
        assert(pindex);
        assert(block->GetHash() == pindex->GetBlockHash());
        if (pindex->nHeight == 0) {
            assert(!pindex->pprev);
            assert(block->hashPrevBlock.IsNull());
        } else {
            assert(pindex->pprev);
            assert(block->hashPrevBlock == pindex->pprev->GetBlockHash());
        }
        assert(WITH_LOCK(::cs_main, return !(pindex->nStatus & BLOCK_FAILED_VALID)));
        m_events.push_back({
            ChainEventType::CONNECTED,
            pindex->GetBlockHash(),
            pindex->pprev ? pindex->pprev->GetBlockHash() : uint256{},
            pindex->nHeight,
        });
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        assert(block);
        assert(pindex);
        assert(pindex->nHeight > 0);
        assert(pindex->pprev);
        assert(block->GetHash() == pindex->GetBlockHash());
        assert(block->hashPrevBlock == pindex->pprev->GetBlockHash());
        m_events.push_back({
            ChainEventType::DISCONNECTED,
            pindex->GetBlockHash(),
            pindex->pprev->GetBlockHash(),
            pindex->nHeight,
        });
    }

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override
    {
        assert(pindexNew);
        assert(WITH_LOCK(::cs_main, return !(pindexNew->nStatus & BLOCK_FAILED_VALID)));
        if (pindexFork) {
            assert(pindexFork->nHeight <= pindexNew->nHeight);
            assert(pindexNew->GetAncestor(pindexFork->nHeight) == pindexFork);
        }
        m_events.push_back({
            ChainEventType::UPDATED_TIP,
            pindexNew->GetBlockHash(),
            pindexFork ? pindexFork->GetBlockHash() : uint256{},
            pindexNew->nHeight,
            pindexFork ? pindexFork->nHeight : -1,
        });
    }

    void ActiveTipChange(const CBlockIndex& new_tip, bool is_ibd) override
    {
        assert(new_tip.nHeight >= 0);
        assert(new_tip.GetAncestor(new_tip.nHeight) == &new_tip);
        m_active_tip_events.push_back({
            new_tip.GetBlockHash(),
            new_tip.nHeight,
        });
    }

    void ChainStateFlushed(const kernel::ChainstateRole&, const CBlockLocator& locator) override
    {
        assert(!locator.IsNull());
        assert(!locator.vHave.empty());
        const CBlockIndex* locator_tip{WITH_LOCK(::cs_main, return Assert(g_setup->m_node.chainman)->m_blockman.LookupBlockIndex(locator.vHave.front()))};
        assert(locator_tip);
        m_events.push_back({
            ChainEventType::FLUSHED,
            locator_tip->GetBlockHash(),
            {},
            locator_tip->nHeight,
        });
    }

    void AssertAndClear(const ChainstateManager& chainman)
    {
        size_t active_tip_idx{0};
        auto consume_active_tip = [&] {
            while (active_tip_idx < m_active_tip_events.size()) {
                const TipEvent& event{m_active_tip_events[active_tip_idx]};
                if (event.hash != m_expected_tip || event.height != m_expected_height) {
                    break;
                }
                ++active_tip_idx;
            }
        };

        consume_active_tip();
        for (const ChainEvent& event : m_events) {
            if (event.type == ChainEventType::CONNECTED) {
                if (!m_update_start_tip) {
                    m_update_start_tip = TipEvent{m_expected_tip, m_expected_height};
                }
                assert(event.prev_hash == m_expected_tip);
                assert(event.height == m_expected_height + 1);
                m_expected_tip = event.hash;
                ++m_expected_height;
            } else if (event.type == ChainEventType::DISCONNECTED) {
                if (!m_update_start_tip) {
                    m_update_start_tip = TipEvent{m_expected_tip, m_expected_height};
                }
                assert(event.hash == m_expected_tip);
                assert(event.height == m_expected_height);
                assert(m_expected_height > 0);
                m_expected_tip = event.prev_hash;
                --m_expected_height;
            } else if (event.type == ChainEventType::UPDATED_TIP) {
                assert(event.hash == m_expected_tip);
                assert(event.height == m_expected_height);
                if (event.fork_height >= 0) {
                    assert(event.fork_height <= event.height);
                    assert(m_update_start_tip);
                    LOCK(chainman.GetMutex());
                    const CBlockIndex* old_tip{chainman.m_blockman.LookupBlockIndex(m_update_start_tip->hash)};
                    const CBlockIndex* new_tip{chainman.m_blockman.LookupBlockIndex(event.hash)};
                    assert(old_tip);
                    assert(new_tip);
                    const CBlockIndex* expected_fork{LastCommonAncestor(old_tip, new_tip)};
                    assert(expected_fork);
                    assert(event.prev_hash == expected_fork->GetBlockHash());
                    assert(event.fork_height == expected_fork->nHeight);
                }
                m_update_start_tip.reset();
            } else {
                assert(event.type == ChainEventType::FLUSHED);
                assert(event.hash == m_expected_tip);
                assert(event.height == m_expected_height);
            }
            consume_active_tip();
        }
        m_update_start_tip.reset();
        assert(active_tip_idx == m_active_tip_events.size());
        LOCK(chainman.GetMutex());
        const CBlockIndex* tip{chainman.ActiveChain().Tip()};
        assert(tip);
        assert(m_expected_tip == tip->GetBlockHash());
        assert(m_expected_height == tip->nHeight);
        m_events.clear();
        m_active_tip_events.clear();
    }
};

std::shared_ptr<const CBlock> MakeDagBlock(
    const CChainParams& params,
    const uint256& prev_hash,
    const int height,
    const uint32_t salt,
    const bool invalid)
{
    CMutableTransaction coinbase_tx;
    coinbase_tx.vin.resize(1);
    coinbase_tx.vin[0].prevout.SetNull();
    coinbase_tx.vin[0].scriptSig = CScript{} << height << salt;
    coinbase_tx.vout.emplace_back(GetBlockSubsidy(height, params.GetConsensus()), CScript{} << OP_TRUE);

    auto block{std::make_shared<CBlock>()};
    block->vtx.push_back(MakeTransactionRef(std::move(coinbase_tx)));

    if (invalid) {
        CMutableTransaction spend_tx;
        spend_tx.vin.emplace_back(COutPoint{block->vtx[0]->GetHash(), 0});
        spend_tx.vout.emplace_back(block->vtx[0]->vout[0].nValue - 1, CScript{} << OP_TRUE);
        block->vtx.push_back(MakeTransactionRef(std::move(spend_tx)));
    }

    block->nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    block->hashPrevBlock = prev_hash;
    block->nTime = params.GenesisBlock().nTime + 100 + salt;
    block->nBits = params.GenesisBlock().nBits;
    block->nNonce = salt;
    block->hashMerkleRoot = BlockMerkleRoot(*block);

    while (!CheckProofOfWork(block->GetHash(), block->nBits, params.GetConsensus())) {
        ++block->nNonce;
        assert(block->nNonce != 0);
    }

    return block;
}

void BuildBlockDag()
{
    const CChainParams& params{Params()};
    const uint256 genesis_hash{params.GenesisBlock().GetHash()};

    const auto a1{MakeDagBlock(params, genesis_hash, 1, 1, false)};
    const auto a2{MakeDagBlock(params, a1->GetHash(), 2, 2, false)};
    const auto a3{MakeDagBlock(params, a2->GetHash(), 3, 3, false)};
    const auto a4{MakeDagBlock(params, a3->GetHash(), 4, 4, false)};

    const auto b1{MakeDagBlock(params, genesis_hash, 1, 101, false)};
    const auto b2{MakeDagBlock(params, b1->GetHash(), 2, 102, false)};
    const auto b3{MakeDagBlock(params, b2->GetHash(), 3, 103, false)};
    const auto b4{MakeDagBlock(params, b3->GetHash(), 4, 104, false)};

    const auto bad_a2{MakeDagBlock(params, a1->GetHash(), 2, 202, true)};
    const auto bad_a3_child{MakeDagBlock(params, bad_a2->GetHash(), 3, 203, false)};

    g_blocks = {
        {a1, true},
        {a2, true},
        {a3, true},
        {a4, true},
        {b1, true},
        {b2, true},
        {b3, true},
        {b4, true},
        {bad_a2, false},
        {bad_a3_child, false},
    };
}

void ResetChainman(TestingSetup& setup)
{
    SetMockTime(Params().GenesisBlock().Time());
    setup.m_node.chainman.reset();
    setup.m_make_chainman();
    setup.LoadVerifyActivateChainstate();
    setup.m_node.validation_signals->SyncWithValidationInterfaceQueue();
}

void AssertActiveChain(const ChainstateManager& chainman)
{
    LOCK(chainman.GetMutex());
    const CChain& chain{chainman.ActiveChain()};
    const CBlockIndex* tip{chain.Tip()};
    assert(tip);
    assert(chain.Height() == tip->nHeight);
    assert(chain[0]);
    assert(chain[0]->pprev == nullptr);
    for (int height{0}; height <= chain.Height(); ++height) {
        const CBlockIndex* index{chain[height]};
        assert(index == tip->GetAncestor(height));
        assert(index->nHeight == height);
        assert(!(index->nStatus & BLOCK_FAILED_VALID));
        if (height > 0) {
            assert(index->pprev == chain[height - 1]);
        }
    }
}

CBlockIndex* LookupBlock(ChainstateManager& chainman, const uint256& hash)
{
    LOCK(chainman.GetMutex());
    return chainman.m_blockman.LookupBlockIndex(hash);
}

bool IsFailedBlock(ChainstateManager& chainman, const CBlockIndex& index)
{
    LOCK(chainman.GetMutex());
    return index.nStatus & BLOCK_FAILED_VALID;
}

bool HasBlockData(ChainstateManager& chainman, const uint256& hash)
{
    LOCK(chainman.GetMutex());
    const CBlockIndex* index{chainman.m_blockman.LookupBlockIndex(hash)};
    return index && ((index->nStatus & BLOCK_HAVE_DATA) != 0);
}

void AssertBlockIndexLineage(const CBlockIndex& index)
{
    if (index.pprev) {
        assert(index.nHeight == index.pprev->nHeight + 1);
        assert(index.nChainWork == index.pprev->nChainWork + GetBlockProof(index));
    } else {
        assert(index.nHeight == 0);
        assert(index.nChainWork == GetBlockProof(index));
    }
}

std::vector<CBlockIndex*> KnownBlockIndexes(ChainstateManager& chainman)
{
    std::vector<CBlockIndex*> indexes;
    LOCK(chainman.GetMutex());
    indexes.push_back(chainman.m_blockman.LookupBlockIndex(Params().GenesisBlock().GetHash()));
    for (const DagBlock& dag_block : g_blocks) {
        if (CBlockIndex* index{chainman.m_blockman.LookupBlockIndex(dag_block.block->GetHash())}) {
            indexes.push_back(index);
        }
    }
    return indexes;
}

void AssertFailedBlockAndDescendants(ChainstateManager& chainman, const CBlockIndex& failed_block)
{
    LOCK(chainman.GetMutex());
    assert(!chainman.ActiveChain().Contains(failed_block));
    assert(failed_block.nStatus & BLOCK_FAILED_VALID);
    for (const auto& [_, index] : chainman.BlockIndex()) {
        if (index.GetAncestor(failed_block.nHeight) == &failed_block) {
            assert(index.nStatus & BLOCK_FAILED_VALID);
        }
    }
}

void AssertBestInvalidIsFailed(const TestChainstateManager& chainman)
{
    LOCK(chainman.GetMutex());
    const CBlockIndex* best_invalid{chainman.BestInvalid()};
    assert(!best_invalid || (best_invalid->nStatus & BLOCK_FAILED_VALID));
}

bool ProcessDagBlock(ChainstateManager& chainman, const DagBlock& dag_block)
{
    bool new_block{false};
    const bool accepted{chainman.ProcessNewBlock(
        dag_block.block,
        /*force_processing=*/true,
        /*min_pow_checked=*/true,
        &new_block)};
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();

    if (dag_block.valid) assert(accepted);
    return accepted;
}

void ProcessDagBlocks(ChainstateManager& chainman, std::initializer_list<size_t> indexes)
{
    for (const size_t index : indexes) {
        ProcessDagBlock(chainman, g_blocks.at(index));
    }
}

void SeedTopology(FuzzedDataProvider& fuzzed_data_provider, ChainstateManager& chainman)
{
    CallOneOf(
        fuzzed_data_provider,
        [&] {
            // Leave only genesis active.
        },
        [&] {
            // Straight empty-block chain.
            ProcessDagBlocks(chainman, {0, 1, 2});
        },
        [&] {
            // Two same-height empty-block branches.
            ProcessDagBlocks(chainman, {0, 1, 2, 4, 5, 6});
        },
        [&] {
            // Longer competing fork, forcing a real disconnect/connect reorg.
            ProcessDagBlocks(chainman, {0, 1, 2, 4, 5, 6, 7});
            LOCK(chainman.GetMutex());
            assert(chainman.ActiveChain().Tip()->GetBlockHash() == g_blocks.at(7).block->GetHash());
        },
        [&] {
            // Valid header plus invalid block, then a descendant of the invalid branch.
            ProcessDagBlocks(chainman, {0, 8, 9});
            if (CBlockIndex* index{LookupBlock(chainman, g_blocks.at(8).block->GetHash())}) {
                AssertFailedBlockAndDescendants(chainman, *index);
            }
        });
}

void ProcessGenesis(ChainstateManager& chainman)
{
    const uint256 genesis_hash{Params().GenesisBlock().GetHash()};
    assert(HasBlockData(chainman, genesis_hash));
    bool new_block{true};
    const bool accepted{chainman.ProcessNewBlock(
        std::make_shared<const CBlock>(Params().GenesisBlock()),
        /*force_processing=*/true,
        /*min_pow_checked=*/true,
        &new_block)};
    assert(accepted);
    assert(!new_block);
    assert(HasBlockData(chainman, genesis_hash));
}
} // namespace

void initialize_validation_block_reorg()
{
    static const auto testing_setup{
        MakeNoLogFileContext<TestingSetup>(
            /*chain_type=*/ChainType::REGTEST,
            {}),
    };
    g_setup = testing_setup.get();
    BuildBlockDag();
    ResetChainman(*g_setup);
}

FUZZ_TARGET(validation_block_reorg, .init = initialize_validation_block_reorg)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    ResetChainman(*g_setup);
    auto& node{g_setup->m_node};
    auto& chainman{static_cast<TestChainstateManager&>(*node.chainman)};
    chainman.ResetIbd();
    chainman.DisableNextWrite();
    const CBlockIndex* initial_tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip())};
    assert(initial_tip);
    auto event_recorder{std::make_shared<ValidationEventRecorder>(*initial_tip)};
    node.validation_signals->RegisterSharedValidationInterface(event_recorder);

    SeedTopology(fuzzed_data_provider, chainman);
    node.validation_signals->SyncWithValidationInterfaceQueue();
    event_recorder->AssertAndClear(chainman);
    AssertActiveChain(chainman);
    AssertBestInvalidIsFailed(chainman);
    chainman.CheckBlockIndex();

    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 32)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                ProcessGenesis(chainman);
            },
            [&] {
                const DagBlock& dag_block{PickValue(fuzzed_data_provider, g_blocks)};
                const bool had_data{HasBlockData(chainman, dag_block.block->GetHash())};
                bool new_block{true};
                const bool pass_new_block_out{fuzzed_data_provider.ConsumeBool()};
                const bool accepted{chainman.ProcessNewBlock(
                    dag_block.block,
                    /*force_processing=*/fuzzed_data_provider.ConsumeBool(),
                    /*min_pow_checked=*/fuzzed_data_provider.ConsumeBool(),
                    pass_new_block_out ? &new_block : nullptr)};
                node.validation_signals->SyncWithValidationInterfaceQueue();

                if (pass_new_block_out) {
                    const bool has_data{HasBlockData(chainman, dag_block.block->GetHash())};
                    assert(!new_block || accepted);
                    assert(!new_block || !had_data);
                    assert(!new_block || has_data);
                    if (!accepted || had_data) {
                        assert(!new_block);
                    }
                }

                if (!dag_block.valid) {
                    if (CBlockIndex* index{LookupBlock(chainman, dag_block.block->GetHash())}) {
                        if (IsFailedBlock(chainman, *index)) {
                            AssertFailedBlockAndDescendants(chainman, *index);
                        }
                    }
                }
            },
            [&] {
                const DagBlock& dag_block{PickValue(fuzzed_data_provider, g_blocks)};
                const uint256 block_hash{dag_block.block->GetHash()};
                const CBlockIndex* returned_index{nullptr};
                const bool pass_index_out{fuzzed_data_provider.ConsumeBool()};
                const bool min_pow_checked{fuzzed_data_provider.ConsumeBool()};
                const bool had_index{LookupBlock(chainman, block_hash) != nullptr};
                const bool had_data{HasBlockData(chainman, block_hash)};
                CBlockIndex* parent_index{LookupBlock(chainman, dag_block.block->hashPrevBlock)};
                const bool parent_failed{parent_index && IsFailedBlock(chainman, *parent_index)};
                const CBlockIndex* old_tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip())};

                BlockValidationState state;
                const bool accepted{chainman.ProcessNewBlockHeaders(
                    {{*dag_block.block}},
                    min_pow_checked,
                    state,
                    pass_index_out ? &returned_index : nullptr)};
                node.validation_signals->SyncWithValidationInterfaceQueue();
                assert(WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip()) == old_tip);

                CBlockIndex* index_after{LookupBlock(chainman, block_hash)};
                if (accepted) {
                    assert(index_after);
                    AssertBlockIndexLineage(*index_after);
                    assert(parent_index);
                    assert(index_after->pprev == parent_index);
                    if (pass_index_out) {
                        assert(returned_index == index_after);
                        assert(returned_index->GetBlockHash() == block_hash);
                    }
                    // Header-only processing must not create block data.
                    if (!had_data) assert(!HasBlockData(chainman, block_hash));
                } else if (!had_index) {
                    assert(!index_after);
                    if (!min_pow_checked && parent_index && !parent_failed) {
                        assert(state.GetResult() == BlockValidationResult::BLOCK_HEADER_LOW_WORK);
                        assert(state.GetRejectReason() == "too-little-chainwork");
                    }
                }
            },
            [&] {
                auto indexes{KnownBlockIndexes(chainman)};
                if (indexes.empty()) return;
                CBlockIndex* index{PickValue(fuzzed_data_provider, indexes)};
                bool was_in_active_chain;
                uint256 old_tip;
                {
                    LOCK(chainman.GetMutex());
                    was_in_active_chain = chainman.ActiveChain().Contains(*index);
                    old_tip = chainman.ActiveChain().Tip()->GetBlockHash();
                }

                BlockValidationState state;
                const bool invalidated{chainman.ActiveChainstate().InvalidateBlock(state, index)};
                node.validation_signals->SyncWithValidationInterfaceQueue();

                if (index->nHeight == 0) {
                    assert(!invalidated);
                    assert(old_tip == WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip()->GetBlockHash()));
                } else {
                    assert(invalidated);
                    AssertFailedBlockAndDescendants(chainman, *index);
                    if (!was_in_active_chain) {
                        assert(old_tip == WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip()->GetBlockHash()));
                    }
                }
            },
            [&] {
                auto indexes{KnownBlockIndexes(chainman)};
                if (indexes.empty()) return;
                CBlockIndex* index{PickValue(fuzzed_data_provider, indexes)};
                std::vector<std::pair<const CBlockIndex*, bool>> failed_before;
                {
                    LOCK(chainman.GetMutex());
                    for (auto& [_, block_index] : chainman.BlockIndex()) {
                        failed_before.emplace_back(&block_index, block_index.nStatus & BLOCK_FAILED_VALID);
                    }

                    chainman.ActiveChainstate().ResetBlockFailureFlags(index);
                    chainman.RecalculateBestHeader();
                    assert(!(index->nStatus & BLOCK_FAILED_VALID));

                    for (const auto& [block, was_failed] : failed_before) {
                        if (IsAncestorOrDescendant(*index, *block)) {
                            assert(!(block->nStatus & BLOCK_FAILED_VALID));
                        } else if (was_failed) {
                            assert(block->nStatus & BLOCK_FAILED_VALID);
                        }
                    }
                }
                BlockValidationState state;
                assert(chainman.ActiveChainstate().ActivateBestChain(state));
                node.validation_signals->SyncWithValidationInterfaceQueue();
            },
            [&] {
                auto indexes{KnownBlockIndexes(chainman)};
                if (indexes.empty()) return;
                CBlockIndex* index{PickValue(fuzzed_data_provider, indexes)};
                const CBlockIndex* old_tip;
                bool lower_work;
                int32_t old_sequence_id;
                {
                    LOCK(chainman.GetMutex());
                    old_tip = chainman.ActiveChain().Tip();
                    lower_work = index->nChainWork < old_tip->nChainWork;
                    old_sequence_id = index->nSequenceId;
                }
                BlockValidationState state;
                assert(chainman.ActiveChainstate().PreciousBlock(state, index));
                node.validation_signals->SyncWithValidationInterfaceQueue();
                if (lower_work) {
                    LOCK(chainman.GetMutex());
                    assert(chainman.ActiveChain().Tip() == old_tip);
                    assert(index->nSequenceId == old_sequence_id);
                }
            },
            [&] {
                BlockValidationState state;
                const FlushStateMode mode{fuzzed_data_provider.ConsumeBool() ?
                                              FlushStateMode::FORCE_FLUSH :
                                              FlushStateMode::FORCE_SYNC};
                assert(chainman.ActiveChainstate().FlushStateToDisk(state, mode));
                node.validation_signals->SyncWithValidationInterfaceQueue();
            });

        node.validation_signals->SyncWithValidationInterfaceQueue();
        event_recorder->AssertAndClear(chainman);
        AssertActiveChain(chainman);
        AssertBestInvalidIsFailed(chainman);
        chainman.CheckBlockIndex();
    }

    node.validation_signals->UnregisterSharedValidationInterface(event_recorder);
    node.validation_signals->SyncWithValidationInterfaceQueue();
}
