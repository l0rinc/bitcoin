// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <array>
#include <memory>

BOOST_FIXTURE_TEST_SUITE(chain_tests, BasicTestingSetup)

namespace {

const CBlockIndex* NaiveGetAncestor(const CBlockIndex* a, int height)
{
    while (a->nHeight > height) {
        a = a->pprev;
    }
    BOOST_REQUIRE_EQUAL(a->nHeight, height);
    return a;
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
        BOOST_REQUIRE_EQUAL(a->nHeight, b->nHeight);
        a = a->pprev;
        b = b->pprev;
    }
    BOOST_REQUIRE_EQUAL(a, b);
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

} // namespace

BOOST_AUTO_TEST_CASE(basic_tests)
{
    // An empty chain
    const CChain chain_0;
    // A chain with 2 blocks
    CChain chain_2;

    CBlockIndex genesis;
    genesis.nHeight = 0;
    chain_2.SetTip(genesis);

    CBlockIndex bi1;
    bi1.pprev = &genesis;
    bi1.nHeight = 1;
    chain_2.SetTip(bi1);

    BOOST_CHECK_EQUAL(chain_0.Height(), -1);
    BOOST_CHECK_EQUAL(chain_2.Height(), 1);

    BOOST_CHECK_EQUAL(chain_0.Tip(), nullptr);
    BOOST_CHECK_EQUAL(chain_2.Tip(), &bi1);

    // Indexer accessor: call with valid and invalid (low & high) values
    BOOST_CHECK_EQUAL(chain_2[-1], nullptr);
    BOOST_CHECK_EQUAL(chain_2[0], &genesis);
    BOOST_CHECK_EQUAL(chain_2[1], &bi1);
    BOOST_CHECK_EQUAL(chain_2[2], nullptr);

    // Contains: call with contained & non-contained blocks
    BOOST_CHECK(chain_2.Contains(genesis));
    BOOST_CHECK(chain_2.Contains(bi1));
    BOOST_CHECK(!chain_0.Contains(genesis));

    // Call with non-tip & tip blocks
    BOOST_CHECK_EQUAL(chain_2.Next(genesis), &bi1);
    BOOST_CHECK_EQUAL(chain_2.Next(bi1), nullptr);
    BOOST_CHECK_EQUAL(chain_0.Next(genesis), nullptr);

    BOOST_CHECK_EQUAL(chain_2.Genesis(), &genesis);
    BOOST_CHECK_EQUAL(chain_0.Genesis(), nullptr);
}

BOOST_AUTO_TEST_CASE(chain_membership_and_locator_contracts)
{
    BOOST_CHECK(LocatorEntries(nullptr).empty());
    BOOST_CHECK(GetLocator(nullptr).IsNull());

    std::array<CBlockIndex, 4> active_blocks;
    for (size_t i{0}; i < active_blocks.size(); ++i) {
        active_blocks[i].nHeight = i;
        active_blocks[i].pprev = i == 0 ? nullptr : &active_blocks[i - 1];
        active_blocks[i].BuildSkip();
    }

    CBlockIndex fork_block_1;
    fork_block_1.nHeight = 1;
    fork_block_1.pprev = &active_blocks[0];
    fork_block_1.BuildSkip();

    CBlockIndex fork_block_2;
    fork_block_2.nHeight = 2;
    fork_block_2.pprev = &fork_block_1;
    fork_block_2.BuildSkip();

    CChain chain;
    chain.SetTip(active_blocks.back());

    BOOST_CHECK(!chain.Contains(fork_block_1));
    BOOST_CHECK(!chain.Contains(fork_block_2));
    BOOST_CHECK_EQUAL(chain.Next(fork_block_1), nullptr);
    BOOST_CHECK_EQUAL(chain.Next(fork_block_2), nullptr);
    BOOST_CHECK_EQUAL(chain.FindFork(fork_block_2), &active_blocks[0]);
}

BOOST_AUTO_TEST_CASE(findfork_tests)
{
    // Create a forking chain
    const auto init_branch{[](auto& blocks, CBlockIndex* parent, int start_height) {
        for (size_t i{0}; i < blocks.size(); ++i) {
            blocks[i].pprev = i == 0 ? parent : &blocks[i - 1];
            blocks[i].nHeight = start_height + i;
        }
    }};

    const auto check_same{[](const CChain& chain, const auto& blocks) {
        for (const auto& block : blocks) {
            BOOST_CHECK_EQUAL(chain.FindFork(block), &block);
        }
    }};

    const auto check_fork_point{[](const CChain& chain, const auto& blocks, const CBlockIndex& fork_point) {
        for (const auto& block : blocks) {
            BOOST_CHECK_EQUAL(chain.FindFork(block), &fork_point);
        }
    }};

    std::array<CBlockIndex, 10> blocks_common;
    init_branch(blocks_common, nullptr, 0);

    std::array<CBlockIndex, 10> blocks_long;
    init_branch(blocks_long, &blocks_common.back(), blocks_common.size());

    std::array<CBlockIndex, 5> blocks_short;
    init_branch(blocks_short, &blocks_common.back(), blocks_common.size());

    // Create a chain with the longer fork
    CChain chain_long;
    chain_long.SetTip(blocks_long.back());
    BOOST_CHECK_EQUAL(chain_long.Height(), 10 + 10 - 1);
    // Test the blocks in the common part -> result should be the same
    check_same(chain_long, blocks_common);
    // Test the blocks on the longer fork -> result should be the same
    check_same(chain_long, blocks_long);
    // Test the blocks on the other shorter fork -> result should be the fork point
    check_fork_point(chain_long, blocks_short, blocks_common.back());

    // Create a chain with the shorter fork
    CChain chain_short;
    chain_short.SetTip(blocks_short.back());
    BOOST_CHECK_EQUAL(chain_short.Height(), 10 + 5 - 1);
    // Test the blocks in the common part -> result should be the same
    check_same(chain_short, blocks_common);
    // Test the blocks on the shorter fork -> result should be the same
    check_same(chain_short, blocks_short);
    // Test the blocks on the other longer fork -> result should be the fork point
    check_fork_point(chain_short, blocks_long, blocks_common.back());

    // Invalid test case. Mixing chains is not supported
    CBlockIndex block_on_unrelated_chain;
    BOOST_CHECK_EQUAL(chain_long.FindFork(block_on_unrelated_chain), nullptr);
}

BOOST_AUTO_TEST_CASE(findearliestatleast_reference_contracts)
{
    std::array<CBlockIndex, 8> blocks;
    const std::array<uint32_t, 8> times{{100, 90, 140, 120, 180, 170, 220, 160}};
    for (size_t i{0}; i < blocks.size(); ++i) {
        blocks[i].nHeight = i;
        blocks[i].pprev = i == 0 ? nullptr : &blocks[i - 1];
        blocks[i].nTime = times[i];
        blocks[i].nTimeMax = i == 0 ? times[i] : std::max(blocks[i - 1].nTimeMax, times[i]);
        blocks[i].BuildSkip();
    }

    CChain chain;
    chain.SetTip(blocks.back());

    const std::array<int64_t, 15> min_times{{-1, 0, 99, 100, 101, 139, 140, 141, 179, 180, 181, 219, 220, 221, int64_t{1} << 33}};
    for (int64_t min_time : min_times) {
        for (int min_height : {-2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 100}) {
            const CBlockIndex* actual{chain.FindEarliestAtLeast(min_time, min_height)};
            const CBlockIndex* expected{NaiveFindEarliestAtLeast(chain, min_time, min_height)};
            BOOST_CHECK_EQUAL(actual, expected);
            if (actual) {
                BOOST_CHECK(chain.Contains(*actual));
                BOOST_CHECK_GE(actual->GetBlockTimeMax(), min_time);
                BOOST_CHECK_GE(actual->nHeight, min_height);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(chain_test)
{
    FastRandomContext ctx;
    std::vector<std::unique_ptr<CBlockIndex>> block_index;
    // Run 10 iterations of the whole test.
    for (int i = 0; i < 10; ++i) {
        block_index.clear();
        // Create genesis block.
        auto genesis = std::make_unique<CBlockIndex>();
        genesis->nHeight = 0;
        block_index.push_back(std::move(genesis));
        // Create 10000 more blocks.
        for (int b = 0; b < 10000; ++b) {
            auto new_index = std::make_unique<CBlockIndex>();
            // 95% of blocks build on top of the last block; the others fork off randomly.
            if (ctx.randrange(20) != 0) {
                new_index->pprev = block_index.back().get();
            } else {
                new_index->pprev = block_index[ctx.randrange(block_index.size())].get();
            }
            new_index->nHeight = new_index->pprev->nHeight + 1;
            new_index->BuildSkip();
            block_index.push_back(std::move(new_index));
        }
        // Run 10000 random GetAncestor queries.
        for (int q = 0; q < 10000; ++q) {
            const CBlockIndex* block = block_index[ctx.randrange(block_index.size())].get();
            unsigned height = ctx.randrange<unsigned>(block->nHeight + 1);
            const CBlockIndex* result = block->GetAncestor(height);
            BOOST_CHECK(result == NaiveGetAncestor(block, height));
        }
        // Run 10000 random LastCommonAncestor queries.
        for (int q = 0; q < 10000; ++q) {
            const CBlockIndex* block1 = block_index[ctx.randrange(block_index.size())].get();
            const CBlockIndex* block2 = block_index[ctx.randrange(block_index.size())].get();
            const CBlockIndex* result = LastCommonAncestor(block1, block2);
            BOOST_CHECK(result == NaiveLastCommonAncestor(block1, block2));
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
