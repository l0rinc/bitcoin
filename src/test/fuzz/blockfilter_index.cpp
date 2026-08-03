// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <chain.h>
#include <index/blockfilterindex.h>
#include <interfaces/chain.h>
#include <node/blockstorage.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/blockfilter.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>
#include <util/check.h>
#include <util/time.h>
#include <validation.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

TestChain100Setup* g_setup{nullptr};

void initialize_blockfilter_index()
{
    static const auto testing_setup = MakeNoLogFileContext<TestChain100Setup>(
        ChainType::REGTEST, TestOpts{.setup_net = false});
    g_setup = testing_setup.get();
}

void CheckFilter(const BlockFilterIndex& index, const CBlockIndex& block_index,
                 const node::BlockManager& blockman)
{
    BlockFilter expected;
    Assert(ComputeFilter(index.GetFilterType(), block_index, expected, blockman));

    BlockFilter actual;
    Assert(index.LookupFilter(&block_index, actual));
    Assert(actual.GetBlockHash() == expected.GetBlockHash());
    Assert(actual.GetEncodedFilter() == expected.GetEncodedFilter());
}

} // namespace

FUZZ_TARGET(blockfilter_index, .init = initialize_blockfilter_index)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    SetMockTime(1231006505);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    auto& setup{*Assert(g_setup)};
    auto& node{setup.m_node};

    BlockFilterIndex index{interfaces::MakeChain(node), BlockFilterType::BASIC,
                           1_MiB, /*f_memory=*/true, /*f_wipe=*/true};
    Assert(index.Init());
    index.Sync();

    std::vector<const CBlockIndex*> chain;
    {
        LOCK(cs_main);
        const CChain& active_chain{node.chainman->ActiveChain()};
        chain.reserve(active_chain.Height() + 1);
        for (const CBlockIndex* block_index{active_chain.Genesis()}; block_index;
             block_index = active_chain.Next(*block_index)) {
            chain.push_back(block_index);
        }
    }
    Assert(!chain.empty());
    const auto& blockman{node.chainman->m_blockman};
    const int tip_height{static_cast<int>(chain.size() - 1)};

    // Exercise the production range and header lookups with an independent
    // filter reconstruction from the block and undo files.
    const int start_height{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, tip_height)};
    const int stop_height{fuzzed_data_provider.ConsumeIntegralInRange<int>(start_height, tip_height)};
    const CBlockIndex& stop_index{*chain.at(stop_height)};

    std::vector<BlockFilter> filters;
    Assert(index.LookupFilterRange(start_height, &stop_index, filters));
    Assert(filters.size() == static_cast<size_t>(stop_height - start_height + 1));
    for (size_t i{0}; i < filters.size(); ++i) {
        BlockFilter expected;
        const CBlockIndex& block_index{*chain.at(start_height + static_cast<int>(i))};
        Assert(ComputeFilter(index.GetFilterType(), block_index, expected, blockman));
        Assert(filters[i].GetBlockHash() == expected.GetBlockHash());
        Assert(filters[i].GetEncodedFilter() == expected.GetEncodedFilter());
    }

    std::vector<uint256> filter_hashes;
    Assert(index.LookupFilterHashRange(start_height, &stop_index, filter_hashes));
    Assert(filter_hashes.size() == filters.size());
    for (size_t i{0}; i < filters.size(); ++i) {
        Assert(filter_hashes[i] == filters[i].GetHash());
    }

    const int single_height{fuzzed_data_provider.ConsumeIntegralInRange<int>(0, tip_height)};
    const CBlockIndex& single_block{*chain.at(single_height)};
    CheckFilter(index, single_block, blockman);

    uint256 previous_header;
    for (int height{0}; height <= single_height; ++height) {
        BlockFilter expected;
        const CBlockIndex& block_index{*chain.at(height)};
        Assert(ComputeFilter(index.GetFilterType(), block_index, expected, blockman));
        previous_header = expected.ComputeHeader(previous_header);
    }
    uint256 actual_header;
    Assert(index.LookupFilterHeader(&single_block, actual_header));
    Assert(actual_header == previous_header);

    std::vector<BlockFilter> invalid_filters;
    std::vector<uint256> invalid_hashes;
    Assert(!index.LookupFilterRange(-1, &stop_index, invalid_filters));
    Assert(!index.LookupFilterHashRange(-1, &stop_index, invalid_hashes));
    Assert(!index.LookupFilterRange(tip_height + 1, &stop_index, invalid_filters));
    Assert(!index.LookupFilterHashRange(tip_height + 1, &stop_index, invalid_hashes));
}
