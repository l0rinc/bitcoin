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
#include <vector>

namespace {
TestingSetup* g_setup;

struct DagBlock {
    std::shared_ptr<const CBlock> block;
    bool valid;
};

std::vector<DagBlock> g_blocks;

struct ChainEvent {
    bool connected;
    uint256 hash;
    uint256 prev_hash;
    int height;
};

struct ValidationEventRecorder final : public CValidationInterface {
    std::vector<ChainEvent> m_events;

    ~ValidationEventRecorder() = default;

    void BlockConnected(const kernel::ChainstateRole& role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex) override
    {
        assert(block);
        assert(pindex);
        assert(block->GetHash() == pindex->GetBlockHash());
        assert(WITH_LOCK(::cs_main, return !(pindex->nStatus & BLOCK_FAILED_VALID)));
        m_events.push_back({
            /*connected=*/true,
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
        m_events.push_back({
            /*connected=*/false,
            pindex->GetBlockHash(),
            pindex->pprev->GetBlockHash(),
            pindex->nHeight,
        });
    }

    void AssertAndClear()
    {
        for (size_t i{1}; i < m_events.size(); ++i) {
            const ChainEvent& previous{m_events.at(i - 1)};
            const ChainEvent& current{m_events.at(i)};
            if (previous.connected && current.connected) {
                assert(current.height == previous.height + 1);
                assert(current.prev_hash == previous.hash);
            } else if (!previous.connected && !current.connected) {
                assert(current.height == previous.height - 1);
                assert(previous.prev_hash == current.hash);
            } else if (!previous.connected && current.connected) {
                assert(current.height <= previous.height);
            }
        }
        m_events.clear();
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
    bool new_block{true};
    const bool accepted{chainman.ProcessNewBlock(std::make_shared<const CBlock>(Params().GenesisBlock()), /*force_processing=*/true, /*min_pow_checked=*/true, &new_block)};
    assert(accepted);
    assert(!new_block);
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
    auto event_recorder{std::make_shared<ValidationEventRecorder>()};
    node.validation_signals->RegisterSharedValidationInterface(event_recorder);

    SeedTopology(fuzzed_data_provider, chainman);
    node.validation_signals->SyncWithValidationInterfaceQueue();
    event_recorder->AssertAndClear();
    AssertActiveChain(chainman);
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
                bool new_block{false};
                (void)chainman.ProcessNewBlock(
                    dag_block.block,
                    /*force_processing=*/fuzzed_data_provider.ConsumeBool(),
                    /*min_pow_checked=*/fuzzed_data_provider.ConsumeBool(),
                    &new_block);
                node.validation_signals->SyncWithValidationInterfaceQueue();

                if (!dag_block.valid) {
                    if (CBlockIndex* index{LookupBlock(chainman, dag_block.block->GetHash())}) {
                        if (IsFailedBlock(chainman, *index)) {
                            AssertFailedBlockAndDescendants(chainman, *index);
                        }
                    }
                }
            },
            [&] {
                auto indexes{KnownBlockIndexes(chainman)};
                if (indexes.empty()) return;
                CBlockIndex* index{PickValue(fuzzed_data_provider, indexes)};
                const uint256 old_tip{WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip()->GetBlockHash())};

                BlockValidationState state;
                const bool invalidated{chainman.ActiveChainstate().InvalidateBlock(state, index)};
                node.validation_signals->SyncWithValidationInterfaceQueue();

                if (index->nHeight == 0) {
                    assert(!invalidated);
                    assert(old_tip == WITH_LOCK(chainman.GetMutex(), return chainman.ActiveChain().Tip()->GetBlockHash()));
                } else {
                    assert(invalidated);
                    AssertFailedBlockAndDescendants(chainman, *index);
                }
            },
            [&] {
                auto indexes{KnownBlockIndexes(chainman)};
                if (indexes.empty()) return;
                CBlockIndex* index{PickValue(fuzzed_data_provider, indexes)};
                {
                    LOCK(chainman.GetMutex());
                    chainman.ActiveChainstate().ResetBlockFailureFlags(index);
                    chainman.RecalculateBestHeader();
                    assert(!(index->nStatus & BLOCK_FAILED_VALID));
                }
                BlockValidationState state;
                assert(chainman.ActiveChainstate().ActivateBestChain(state));
                node.validation_signals->SyncWithValidationInterfaceQueue();
            },
            [&] {
                auto indexes{KnownBlockIndexes(chainman)};
                if (indexes.empty()) return;
                CBlockIndex* index{PickValue(fuzzed_data_provider, indexes)};
                BlockValidationState state;
                assert(chainman.ActiveChainstate().PreciousBlock(state, index));
                node.validation_signals->SyncWithValidationInterfaceQueue();
            });

        node.validation_signals->SyncWithValidationInterfaceQueue();
        event_recorder->AssertAndClear();
        AssertActiveChain(chainman);
        chainman.CheckBlockIndex();
    }

    node.validation_signals->UnregisterSharedValidationInterface(event_recorder);
    node.validation_signals->SyncWithValidationInterfaceQueue();
}
