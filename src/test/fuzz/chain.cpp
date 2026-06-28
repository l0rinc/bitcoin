// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <arith_uint256.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace {

const CBlockIndex* NaiveGetAncestor(const CBlockIndex* block, int height)
{
    while (block && block->nHeight > height) {
        block = block->pprev;
    }
    return block && block->nHeight == height ? block : nullptr;
}

const CBlockIndex* NaiveLastCommonAncestor(const CBlockIndex* a, const CBlockIndex* b)
{
    while (a->nHeight > b->nHeight) {
        a = a->pprev;
    }
    while (b->nHeight > a->nHeight) {
        b = b->pprev;
    }
    while (a != b) {
        assert(a->nHeight == b->nHeight);
        a = a->pprev;
        b = b->pprev;
    }
    return a;
}

const CBlockIndex* NaiveFindEarliestAtLeast(const CChain& chain, int64_t min_time, int min_height)
{
    for (int height{std::max(min_height, 0)}; height <= chain.Height(); ++height) {
        const CBlockIndex* block{chain[height]};
        if (block->GetBlockTimeMax() >= min_time) return block;
    }
    return nullptr;
}

std::vector<const CBlockIndex*> NaiveLocatorIndexes(const CBlockIndex* block)
{
    int step{1};
    std::vector<const CBlockIndex*> indexes;
    while (block) {
        indexes.push_back(block);
        if (block->nHeight == 0) break;
        block = NaiveGetAncestor(block, std::max(block->nHeight - step, 0));
        if (indexes.size() > 10) step *= 2;
    }
    return indexes;
}

void AssertChainContracts(FuzzedDataProvider& fuzzed_data_provider)
{
    const size_t block_count{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 512)};
    std::vector<uint256> hashes;
    hashes.reserve(block_count);
    std::vector<std::unique_ptr<CBlockIndex>> blocks;
    blocks.reserve(block_count);

    hashes.push_back(ArithToUint256(0));
    auto genesis{std::make_unique<CBlockIndex>()};
    genesis->nHeight = 0;
    genesis->nTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    genesis->nTimeMax = genesis->nTime;
    genesis->phashBlock = &hashes.back();
    genesis->BuildSkip();
    blocks.push_back(std::move(genesis));

    for (size_t i{1}; i < block_count; ++i) {
        const size_t parent_pos{fuzzed_data_provider.ConsumeBool() ? i - 1 : fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, blocks.size() - 1)};
        hashes.push_back(ArithToUint256(i));
        auto block{std::make_unique<CBlockIndex>()};
        block->pprev = blocks[parent_pos].get();
        block->nHeight = block->pprev->nHeight + 1;
        block->nTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
        block->nTimeMax = std::max(block->pprev->nTimeMax, block->nTime);
        block->phashBlock = &hashes.back();
        block->BuildSkip();
        blocks.push_back(std::move(block));
    }

    CBlockIndex* active_tip{blocks[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, blocks.size() - 1)].get()};
    CChain chain;
    chain.SetTip(*active_tip);
    assert(chain.Tip() == active_tip);
    assert(chain.Height() == active_tip->nHeight);
    assert(chain.Genesis() == blocks.front().get());
    assert(chain[-1] == nullptr);
    assert(chain[chain.Height() + 1] == nullptr);

    for (int height{0}; height <= chain.Height(); ++height) {
        const CBlockIndex* ancestor{NaiveGetAncestor(active_tip, height)};
        assert(chain[height] == ancestor);
        assert(chain[height]->nHeight == height);
        assert(chain.Contains(*ancestor));
        assert(chain.FindFork(*ancestor) == ancestor);
        assert(chain.Next(*ancestor) == (height == chain.Height() ? nullptr : NaiveGetAncestor(active_tip, height + 1)));
    }

    for (const auto& block : blocks) {
        const bool in_active_chain{block->nHeight <= chain.Height() && NaiveGetAncestor(active_tip, block->nHeight) == block.get()};
        assert(chain.Contains(*block) == in_active_chain);
        assert(chain.FindFork(*block) == NaiveLastCommonAncestor(active_tip, block.get()));
        if (in_active_chain && block.get() != active_tip) {
            assert(chain.Next(*block) == NaiveGetAncestor(active_tip, block->nHeight + 1));
        } else {
            assert(chain.Next(*block) == nullptr);
        }
    }

    for (unsigned query{0}; query < 16; ++query) {
        const int64_t min_time{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
        const int min_height{fuzzed_data_provider.ConsumeIntegral<int>()};
        const CBlockIndex* actual{chain.FindEarliestAtLeast(min_time, min_height)};
        const CBlockIndex* expected{NaiveFindEarliestAtLeast(chain, min_time, min_height)};
        assert(actual == expected);
        if (actual) {
            assert(chain.Contains(*actual));
            assert(actual->GetBlockTimeMax() >= min_time);
            assert(actual->nHeight >= min_height);
        }
    }

    const CBlockIndex* locator_tip{blocks[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, blocks.size() - 1)].get()};
    const std::vector<uint256> locator_entries{LocatorEntries(locator_tip)};
    const std::vector<const CBlockIndex*> expected_locator_indexes{NaiveLocatorIndexes(locator_tip)};
    assert(GetLocator(locator_tip).vHave == locator_entries);
    assert(locator_entries.size() == expected_locator_indexes.size());
    for (size_t i{0}; i < locator_entries.size(); ++i) {
        assert(locator_entries[i] == expected_locator_indexes[i]->GetBlockHash());
        if (i > 0) {
            assert(expected_locator_indexes[i]->nHeight < expected_locator_indexes[i - 1]->nHeight);
        }
    }
    assert(locator_entries.front() == locator_tip->GetBlockHash());
    assert(locator_entries.back() == blocks.front()->GetBlockHash());
    assert(LocatorEntries(nullptr).empty());
    assert(GetLocator(nullptr).IsNull());
}

} // namespace

FUZZ_TARGET(chain)
{
    FuzzedDataProvider chain_provider(buffer.data(), buffer.size());
    AssertChainContracts(chain_provider);

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::optional<CDiskBlockIndex> disk_block_index = ConsumeDeserializable<CDiskBlockIndex>(fuzzed_data_provider);
    if (!disk_block_index) {
        return;
    }

    const uint256 zero{};
    disk_block_index->phashBlock = &zero;
    {
        LOCK(::cs_main);
        (void)disk_block_index->ConstructBlockHash();
        (void)disk_block_index->GetBlockPos();
        (void)disk_block_index->GetBlockTime();
        (void)disk_block_index->GetBlockTimeMax();
        (void)disk_block_index->GetMedianTimePast();
        (void)disk_block_index->GetUndoPos();
        (void)disk_block_index->HaveNumChainTxs();
        (void)disk_block_index->IsValid(BLOCK_VALID_TRANSACTIONS);
    }

    const CBlockHeader block_header = disk_block_index->GetBlockHeader();
    (void)CDiskBlockIndex{*disk_block_index};
    (void)disk_block_index->BuildSkip();

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const BlockStatus block_status = fuzzed_data_provider.PickValueInArray({
            BlockStatus::BLOCK_VALID_UNKNOWN,
            BlockStatus::BLOCK_VALID_RESERVED,
            BlockStatus::BLOCK_VALID_TREE,
            BlockStatus::BLOCK_VALID_TRANSACTIONS,
            BlockStatus::BLOCK_VALID_CHAIN,
            BlockStatus::BLOCK_VALID_SCRIPTS,
            BlockStatus::BLOCK_VALID_MASK,
            BlockStatus::BLOCK_HAVE_DATA,
            BlockStatus::BLOCK_HAVE_UNDO,
            BlockStatus::BLOCK_HAVE_MASK,
            BlockStatus::BLOCK_FAILED_VALID,
            BlockStatus::BLOCK_OPT_WITNESS,
        });
        if (block_status & ~BLOCK_VALID_MASK) {
            continue;
        }
        WITH_LOCK(::cs_main, (void)disk_block_index->RaiseValidity(block_status));
    }

    CBlockIndex block_index{block_header};
    block_index.phashBlock = &zero;
    (void)block_index.GetBlockHash();
    (void)block_index.ToString();
}
