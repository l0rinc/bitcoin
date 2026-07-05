// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <txgraph.h>

#include <random.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <memory>
#include <set>
#include <vector>

BOOST_AUTO_TEST_SUITE(txgraph_tests)

namespace {

/** The number used as acceptable_cost argument in these tests. High enough that everything
 *  should be optimal, always. */
constexpr uint64_t HIGH_ACCEPTABLE_COST = 100'000'000;

std::strong_ordering PointerComparator(const TxGraph::Ref& a, const TxGraph::Ref& b) noexcept
{
    return (&a) <=> (&b);
}

void CheckDiagram(const std::vector<FeeFrac>& actual, const std::vector<FeeFrac>& expected)
{
    BOOST_REQUIRE_EQUAL(actual.size(), expected.size());
    for (size_t i{0}; i < expected.size(); ++i) {
        BOOST_CHECK_EQUAL(actual[i].fee, expected[i].fee);
        BOOST_CHECK_EQUAL(actual[i].size, expected[i].size);
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(txgraph_trim_zigzag)
{
    // T     T     T     T     T     T     T     T     T     T     T     T     T     T (50 T's)
    //  \   / \   / \   / \   / \   / \   / \   / \   / \   / \   / \   / \   / \   /
    //   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /   \ /
    //    B     B     B     B     B     B     B     B     B     B     B     B     B    (49 B's)
    //
    /** The maximum cluster count used in this test. */
    static constexpr int MAX_CLUSTER_COUNT = 50;
    /** The number of "bottom" transactions, which are in the mempool already. */
    static constexpr int NUM_BOTTOM_TX = 49;
    /** The number of "top" transactions, which come from disconnected blocks. These are re-added
     *  to the mempool and, while connecting them to the already-in-mempool transactions, we
     *   discover the resulting cluster is oversized. */
    static constexpr int NUM_TOP_TX = 50;
    /** The total number of transactions in the test. */
    static constexpr int NUM_TOTAL_TX = NUM_BOTTOM_TX + NUM_TOP_TX;
    static_assert(NUM_TOTAL_TX > MAX_CLUSTER_COUNT);
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;

    // Create a new graph for the test.
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    // Add all transactions and store their Refs.
    std::vector<TxGraph::Ref> refs;
    refs.reserve(NUM_TOTAL_TX);
    // First all bottom transactions: the i'th bottom transaction is at position i.
    for (unsigned int i = 0; i < NUM_BOTTOM_TX; ++i) {
        graph->AddTransaction(refs.emplace_back(), FeePerWeight{200 - i, 100});
    }
    // Then all top transactions: the i'th top transaction is at position NUM_BOTTOM_TX + i.
    for (unsigned int i = 0; i < NUM_TOP_TX; ++i) {
        graph->AddTransaction(refs.emplace_back(), FeePerWeight{100 - i, 100});
    }

    // Create the zigzag dependency structure.
    // Each transaction in the bottom row depends on two adjacent transactions from the top row.
    graph->SanityCheck();
    for (unsigned int i = 0; i < NUM_BOTTOM_TX; ++i) {
        graph->AddDependency(/*parent=*/refs[NUM_BOTTOM_TX + i], /*child=*/refs[i]);
        graph->AddDependency(/*parent=*/refs[NUM_BOTTOM_TX + i + 1], /*child=*/refs[i]);
    }

    // Check that the graph is now oversized. This also forces the graph to
    // group clusters and compute the oversized status.
    graph->SanityCheck();
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), NUM_TOTAL_TX);
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    graph->SanityCheck();
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // We only need to trim the middle bottom transaction to end up with 2 clusters each within cluster limits.
    BOOST_CHECK_EQUAL(removed_refs.size(), 1);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), MAX_CLUSTER_COUNT * 2 - 2);
    for (unsigned int i = 0; i < refs.size(); ++i) {
        BOOST_CHECK_EQUAL(graph->Exists(refs[i], TxGraph::Level::TOP), i != (NUM_BOTTOM_TX / 2));
    }
}

BOOST_AUTO_TEST_CASE(txgraph_trim_flower)
{
    // We will build an oversized flower-shaped graph: all transactions are spent by 1 descendant.
    //
    //   T   T   T   T   T   T   T   T (100 T's)
    //   |   |   |   |   |   |   |   |
    //   |   |   |   |   |   |   |   |
    //   \---+---+---+-+-+---+---+---/
    //                 |
    //                 B (1 B)
    //
    /** The maximum cluster count used in this test. */
    static constexpr int MAX_CLUSTER_COUNT = 50;
    /** The number of "top" transactions, which come from disconnected blocks. These are re-added
     *  to the mempool and, connecting them to the already-in-mempool transactions, we discover the
     *  resulting cluster is oversized. */
    static constexpr int NUM_TOP_TX = MAX_CLUSTER_COUNT * 2;
    /** The total number of transactions in this test. */
    static constexpr int NUM_TOTAL_TX = NUM_TOP_TX + 1;
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;

    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    // Add all transactions and store their Refs.
    std::vector<TxGraph::Ref> refs;
    refs.reserve(NUM_TOTAL_TX);

    // Add all transactions. They are in individual clusters.
    graph->AddTransaction(refs.emplace_back(), {1, 100});
    for (unsigned int i = 0; i < NUM_TOP_TX; ++i) {
        graph->AddTransaction(refs.emplace_back(), FeePerWeight{500 + i, 100});
    }
    graph->SanityCheck();

    // The 0th transaction spends all the top transactions.
    for (unsigned int i = 1; i < NUM_TOTAL_TX; ++i) {
        graph->AddDependency(/*parent=*/refs[i], /*child=*/refs[0]);
    }
    graph->SanityCheck();

    // Check that the graph is now oversized. This also forces the graph to
    // group clusters and compute the oversized status.
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    graph->SanityCheck();
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // Since only the bottom transaction connects these clusters, we only need to remove it.
    BOOST_CHECK_EQUAL(removed_refs.size(), 1);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), MAX_CLUSTER_COUNT * 2);
    BOOST_CHECK(!graph->Exists(refs[0], TxGraph::Level::TOP));
    for (unsigned int i = 1; i < refs.size(); ++i) {
        BOOST_CHECK(graph->Exists(refs[i], TxGraph::Level::TOP));
    }
}

BOOST_AUTO_TEST_CASE(txgraph_trim_huge)
{
    // The from-block transactions consist of 1000 fully linear clusters, each with 64
    // transactions. The mempool contains 11 transactions that together merge all of these into
    // a single cluster.
    //
    // (1000 chains of 64 transactions, 64000 T's total)
    //
    //      T          T          T          T          T          T          T          T
    //      |          |          |          |          |          |          |          |
    //      T          T          T          T          T          T          T          T
    //      |          |          |          |          |          |          |          |
    //      T          T          T          T          T          T          T          T
    //      |          |          |          |          |          |          |          |
    //      T          T          T          T          T          T          T          T
    //  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)  (64 long)
    //      |          |          |          |          |          |          |          |
    //      |          |         / \         |         / \         |          |         /
    //      \----------+--------/   \--------+--------/   \--------+-----+----+--------/
    //                 |                     |                           |
    //                 B                     B                           B
    //
    //  (11 B's, each attaching to up to 100 chains of 64 T's)
    //
    /** The maximum cluster count used in this test. */
    static constexpr int MAX_CLUSTER_COUNT = 64;
    /** The number of "top" (from-block) chains of transactions. */
    static constexpr int NUM_TOP_CHAINS = 1000;
    /** The number of transactions per top chain. */
    static constexpr int NUM_TX_PER_TOP_CHAIN = MAX_CLUSTER_COUNT;
    /** The (maximum) number of dependencies per bottom transaction. */
    static constexpr int NUM_DEPS_PER_BOTTOM_TX = 100;
    /** The number of bottom transactions that are expected to be created. */
    static constexpr int NUM_BOTTOM_TX = (NUM_TOP_CHAINS - 1 + (NUM_DEPS_PER_BOTTOM_TX - 2)) / (NUM_DEPS_PER_BOTTOM_TX - 1);
    /** The total number of transactions created in this test. */
    static constexpr int NUM_TOTAL_TX = NUM_TOP_CHAINS * NUM_TX_PER_TOP_CHAIN + NUM_BOTTOM_TX;
    /** Set a very large cluster size limit so that only the count limit is triggered. */
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000 * 100;

    /** Refs to all top transactions. */
    std::vector<TxGraph::Ref> top_refs;
    /** Refs to all bottom transactions. */
    std::vector<TxGraph::Ref> bottom_refs;
    /** Indexes into top_refs for some transaction of each component, in arbitrary order.
     *  Initially these are the last transactions in each chains, but as bottom transactions are
     *  added, entries will be removed when they get merged, and randomized. */
    std::vector<size_t> top_components;

    FastRandomContext rng;
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    // Construct the top chains.
    for (int chain = 0; chain < NUM_TOP_CHAINS; ++chain) {
        for (int chaintx = 0; chaintx < NUM_TX_PER_TOP_CHAIN; ++chaintx) {
            // Use random fees, size 1.
            int64_t fee = rng.randbits<27>() + 100;
            FeePerWeight feerate{fee, 1};
            graph->AddTransaction(top_refs.emplace_back(), feerate);
            // Add internal dependencies linking the chain transactions together.
            if (chaintx > 0) {
                 graph->AddDependency(*(top_refs.rbegin()), *(top_refs.rbegin() + 1));
            }
        }
        // Remember the last transaction in each chain, to attach the bottom transactions to.
        top_components.push_back(top_refs.size() - 1);
    }
    graph->SanityCheck();

    // Not oversized so far (just 1000 clusters of 64).
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // Construct the bottom transactions, and dependencies to the top chains.
    while (top_components.size() > 1) {
        // Construct the transaction.
        int64_t fee = rng.randbits<27>() + 100;
        FeePerWeight feerate{fee, 1};
        TxGraph::Ref bottom_tx;
        graph->AddTransaction(bottom_tx, feerate);
        // Determine the number of dependencies this transaction will have.
        int deps = std::min<int>(NUM_DEPS_PER_BOTTOM_TX, top_components.size());
        for (int dep = 0; dep < deps; ++dep) {
            // Pick an transaction in top_components to attach to.
            auto idx = rng.randrange(top_components.size());
            // Add dependency.
            graph->AddDependency(/*parent=*/top_refs[top_components[idx]], /*child=*/bottom_tx);
            // Unless this is the last dependency being added, remove from top_components, as
            // the component will be merged with that one.
            if (dep < deps - 1) {
                // Move entry top the back.
                if (idx != top_components.size() - 1) std::swap(top_components.back(), top_components[idx]);
                // And pop it.
                top_components.pop_back();
            }
        }
        bottom_refs.push_back(std::move(bottom_tx));
    }
    graph->SanityCheck();

    // Now we are oversized (one cluster of 64011).
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));
    const auto total_tx_count = graph->GetTransactionCount(TxGraph::Level::TOP);
    BOOST_CHECK(total_tx_count == top_refs.size() + bottom_refs.size());
    BOOST_CHECK(total_tx_count == NUM_TOTAL_TX);

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));
    BOOST_CHECK(removed_refs.size() == total_tx_count - graph->GetTransactionCount(TxGraph::Level::TOP));
    graph->SanityCheck();

    // At least 99% of chains must survive.
    BOOST_CHECK(graph->GetTransactionCount(TxGraph::Level::TOP) >= (NUM_TOP_CHAINS * NUM_TX_PER_TOP_CHAIN * 99) / 100);
}

BOOST_AUTO_TEST_CASE(txgraph_trim_big_singletons)
{
    // Mempool consists of 100 singleton clusters; there are no dependencies. Some are oversized. Trim() should remove all of the oversized ones.
    static constexpr int MAX_CLUSTER_COUNT = 64;
    static constexpr int32_t MAX_CLUSTER_SIZE = 100'000;
    static constexpr int NUM_TOTAL_TX = 100;

    // Create a new graph for the test.
    auto graph = MakeTxGraph(MAX_CLUSTER_COUNT, MAX_CLUSTER_SIZE, HIGH_ACCEPTABLE_COST, PointerComparator);

    // Add all transactions and store their Refs.
    std::vector<TxGraph::Ref> refs;
    refs.reserve(NUM_TOTAL_TX);

    // Add all transactions. They are in individual clusters.
    for (unsigned int i = 0; i < NUM_TOTAL_TX; ++i) {
        // The 88th transaction is oversized.
        // Every 20th transaction is oversized.
        const FeePerWeight feerate{500 + i, (i == 88 || i % 20 == 0) ? MAX_CLUSTER_SIZE + 1 : 100};
        graph->AddTransaction(refs.emplace_back(), feerate);
    }
    graph->SanityCheck();

    // Check that the graph is now oversized. This also forces the graph to
    // group clusters and compute the oversized status.
    BOOST_CHECK(graph->IsOversized(TxGraph::Level::TOP));

    // Call Trim() to remove transactions and bring the cluster back within limits.
    auto removed_refs = graph->Trim();
    graph->SanityCheck();
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), NUM_TOTAL_TX - 6);
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::TOP));

    // Check that all the oversized transactions were removed.
    for (unsigned int i = 0; i < refs.size(); ++i) {
        BOOST_CHECK_EQUAL(graph->Exists(refs[i], TxGraph::Level::TOP), i != 88 && i % 20 != 0);
    }
}

BOOST_AUTO_TEST_CASE(txgraph_chunk_chain)
{
    // Create a new graph for the test.
    auto graph = MakeTxGraph(50, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    auto block_builder_checker = [&graph](std::vector<std::vector<TxGraph::Ref*>> expected_chunks) {
        std::vector<std::vector<TxGraph::Ref*>> chunks;
        auto builder = graph->GetBlockBuilder();
        FeePerWeight last_chunk_feerate;
        while (auto chunk = builder->GetCurrentChunk()) {
            FeePerWeight sum;
            for (TxGraph::Ref* ref : chunk->first) {
                // The reported chunk feerate must match the chunk feerate obtained by asking
                // it for each of the chunk's transactions individually.
                BOOST_CHECK(graph->GetMainChunkFeerate(*ref) == chunk->second);
                // Verify the chunk feerate matches the sum of the reported individual feerates.
                sum += graph->GetIndividualFeerate(*ref);
            }
            BOOST_CHECK(sum == chunk->second);
            chunks.push_back(std::move(chunk->first));
            last_chunk_feerate = chunk->second;
            builder->Include();
        }

        BOOST_CHECK(chunks == expected_chunks);
        auto& last_chunk = chunks.back();
        // The last chunk returned by the BlockBuilder must match GetWorstMainChunk, in reverse.
        std::reverse(last_chunk.begin(), last_chunk.end());
        auto [worst_chunk, worst_chunk_feerate] = graph->GetWorstMainChunk();
        BOOST_CHECK(last_chunk == worst_chunk);
        BOOST_CHECK(last_chunk_feerate == worst_chunk_feerate);
    };

    std::vector<TxGraph::Ref> refs;
    refs.reserve(4);

    FeePerWeight feerateA{2, 10};
    FeePerWeight feerateB{1, 10};
    FeePerWeight feerateC{2, 10};
    FeePerWeight feerateD{4, 10};

    // everytime adding a transaction, test the chunk status
    // [A]
    graph->AddTransaction(refs.emplace_back(), feerateA);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);
    block_builder_checker({{&refs[0]}});
    // [A, B]
    graph->AddTransaction(refs.emplace_back(), feerateB);
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 2);
    block_builder_checker({{&refs[0]}, {&refs[1]}});

    // [A, BC]
    graph->AddTransaction(refs.emplace_back(), feerateC);
    graph->AddDependency(/*parent=*/refs[1], /*child=*/refs[2]);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 3);
    block_builder_checker({{&refs[0]}, {&refs[1], &refs[2]}});

    // [ABCD]
    graph->AddTransaction(refs.emplace_back(), feerateD);
    graph->AddDependency(/*parent=*/refs[2], /*child=*/refs[3]);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 4);
    block_builder_checker({{&refs[0], &refs[1], &refs[2], &refs[3]}});

    graph->SanityCheck();

    // D->C->A
    graph->RemoveTransaction(refs[1]);
    // txgraph is not responsible for removing the descendants or ancestors
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 3);
    // only A remains there
    graph->RemoveTransaction(refs[2]);
    graph->RemoveTransaction(refs[3]);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);
    block_builder_checker({{&refs[0]}});
}

BOOST_AUTO_TEST_CASE(txgraph_block_builder_skip_excludes_cluster)
{
    auto graph = MakeTxGraph(50, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(3);

    graph->AddTransaction(refs.emplace_back(), FeePerWeight{10, 1}); // A
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{1, 1});  // B
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{5, 1});  // C
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    graph->SanityCheck();

    {
        auto builder = graph->GetBlockBuilder();
        auto first_chunk = builder->GetCurrentChunk();
        BOOST_REQUIRE(first_chunk);
        BOOST_REQUIRE_EQUAL(first_chunk->first.size(), 1U);
        BOOST_CHECK_EQUAL(first_chunk->first[0], &refs[0]);
        BOOST_CHECK((first_chunk->second == FeePerWeight{10, 1}));
    }

    {
        auto builder = graph->GetBlockBuilder();
        builder->Skip();

        auto second_chunk = builder->GetCurrentChunk();
        BOOST_REQUIRE(second_chunk);
        BOOST_REQUIRE_EQUAL(second_chunk->first.size(), 1U);
        BOOST_CHECK_EQUAL(second_chunk->first[0], &refs[2]);
        BOOST_CHECK((second_chunk->second == FeePerWeight{5, 1}));

        builder->Include();
        BOOST_CHECK(!builder->GetCurrentChunk());
    }
}

BOOST_AUTO_TEST_CASE(txgraph_block_builder_exhaustion_is_stable)
{
    auto graph = MakeTxGraph(50, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    TxGraph::Ref ref;
    graph->AddTransaction(ref, FeePerWeight{10, 1});

    auto builder = graph->GetBlockBuilder();
    BOOST_REQUIRE(builder->GetCurrentChunk());
    builder->Include();
    BOOST_CHECK(!builder->GetCurrentChunk());

    builder->Include();
    BOOST_CHECK(!builder->GetCurrentChunk());

    builder->Skip();
    BOOST_CHECK(!builder->GetCurrentChunk());
}

BOOST_AUTO_TEST_CASE(txgraph_block_builder_current_chunk_ref_contract)
{
    auto graph = MakeTxGraph(50, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(4);

    for (const FeePerWeight& feerate : {FeePerWeight{2, 10}, FeePerWeight{1, 10}, FeePerWeight{2, 10}, FeePerWeight{4, 10}}) {
        graph->AddTransaction(refs.emplace_back(), feerate);
    }
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    graph->AddDependency(/*parent=*/refs[1], /*child=*/refs[2]);
    graph->AddDependency(/*parent=*/refs[2], /*child=*/refs[3]);

    auto builder = graph->GetBlockBuilder();
    auto chunk = builder->GetCurrentChunk();
    BOOST_REQUIRE(chunk);
    BOOST_REQUIRE_EQUAL(chunk->first.size(), refs.size());
    BOOST_CHECK((chunk->second == FeePerWeight{9, 40}));

    FeePerWeight sum;
    for (TxGraph::Ref* ref : chunk->first) {
        BOOST_REQUIRE(ref);
        BOOST_CHECK(graph->Exists(*ref, TxGraph::Level::MAIN));
        BOOST_CHECK(graph->GetMainChunkFeerate(*ref) == chunk->second);
        sum += graph->GetIndividualFeerate(*ref);
    }
    BOOST_CHECK(sum == chunk->second);
    BOOST_CHECK(chunk->first == std::vector<TxGraph::Ref*>({&refs[0], &refs[1], &refs[2], &refs[3]}));

    builder->Include();
    BOOST_CHECK(!builder->GetCurrentChunk());
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_getcluster_membership_contracts)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(5);

    for (const FeePerWeight& feerate : {FeePerWeight{10, 10}, FeePerWeight{4, 10}, FeePerWeight{8, 10}, FeePerWeight{1, 10}}) {
        graph->AddTransaction(refs.emplace_back(), feerate);
    }
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    graph->AddDependency(/*parent=*/refs[1], /*child=*/refs[2]);

    auto check_cluster = [&graph](TxGraph::Ref& ref, TxGraph::Level level, const std::vector<TxGraph::Ref*>& expected) {
        const auto cluster{graph->GetCluster(ref, level)};
        BOOST_CHECK(cluster == expected);
        const std::set<TxGraph::Ref*> unique_cluster_refs{cluster.begin(), cluster.end()};
        BOOST_CHECK_EQUAL(unique_cluster_refs.size(), cluster.size());
        for (TxGraph::Ref* member : cluster) {
            BOOST_REQUIRE(member);
            BOOST_CHECK(graph->GetCluster(*member, level) == cluster);
        }
    };

    const std::vector<TxGraph::Ref*> main_cluster{&refs[0], &refs[1], &refs[2]};
    check_cluster(refs[0], TxGraph::Level::MAIN, main_cluster);
    check_cluster(refs[1], TxGraph::Level::TOP, main_cluster);
    check_cluster(refs[3], TxGraph::Level::TOP, {&refs[3]});

    TxGraph::Ref empty_ref;
    BOOST_CHECK(graph->GetCluster(empty_ref, TxGraph::Level::TOP).empty());

    graph->StartStaging();
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{9, 10});
    graph->AddDependency(/*parent=*/refs[2], /*child=*/refs[4]);

    check_cluster(refs[4], TxGraph::Level::TOP, {&refs[0], &refs[1], &refs[2], &refs[4]});
    check_cluster(refs[2], TxGraph::Level::MAIN, main_cluster);
    BOOST_CHECK(graph->GetCluster(refs[4], TxGraph::Level::MAIN).empty());

    graph->CommitStaging();
    check_cluster(refs[4], TxGraph::Level::MAIN, {&refs[0], &refs[1], &refs[2], &refs[4]});
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_count_distinct_clusters_contracts)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(7);
    for (const FeePerWeight& feerate : {FeePerWeight{10, 10}, FeePerWeight{9, 10}, FeePerWeight{8, 10}, FeePerWeight{7, 10}, FeePerWeight{6, 10}}) {
        graph->AddTransaction(refs.emplace_back(), feerate);
    }
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    graph->AddDependency(/*parent=*/refs[2], /*child=*/refs[3]);

    TxGraph::Ref empty_ref;
    auto check_count = [&](std::vector<TxGraph::Ref*> query, TxGraph::Level level, uint32_t expected) {
        BOOST_CHECK_EQUAL(graph->CountDistinctClusters(query, level), expected);
        std::ranges::reverse(query);
        BOOST_CHECK_EQUAL(graph->CountDistinctClusters(query, level), expected);
        query.push_back(&empty_ref);
        if (!query.empty()) {
            query.push_back(query.front());
            query.push_back(query.back());
        }
        BOOST_CHECK_EQUAL(graph->CountDistinctClusters(query, level), expected);
    };

    check_count({}, TxGraph::Level::TOP, 0);
    check_count({&empty_ref}, TxGraph::Level::TOP, 0);
    check_count({&refs[1], &refs[0], &refs[0], &refs[3], &empty_ref}, TxGraph::Level::TOP, 2);

    graph->StartStaging();
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{5, 10});
    graph->AddDependency(/*parent=*/refs[1], /*child=*/refs[5]);
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{4, 10});

    check_count({&refs[5], &refs[0], &refs[3], &refs[6], &empty_ref}, TxGraph::Level::TOP, 3);
    check_count({&refs[5], &refs[0], &refs[3], &refs[6], &empty_ref}, TxGraph::Level::MAIN, 2);

    graph->CommitStaging();
    check_count({&refs[5], &refs[0], &refs[3], &refs[6], &empty_ref}, TxGraph::Level::MAIN, 3);
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_compare_main_order_contracts)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(3);
    for (const FeePerWeight& feerate : {FeePerWeight{3, 10}, FeePerWeight{1, 10}, FeePerWeight{2, 10}}) {
        graph->AddTransaction(refs.emplace_back(), feerate);
    }
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    graph->SanityCheck();

    auto check_pair = [&](TxGraph::Ref& a, TxGraph::Ref& b) {
        const auto ab{graph->CompareMainOrder(a, b)};
        const auto ba{graph->CompareMainOrder(b, a)};
        if (&a == &b) {
            BOOST_CHECK(ab == std::strong_ordering::equal);
            BOOST_CHECK(ba == std::strong_ordering::equal);
        } else if (ab == std::strong_ordering::less) {
            BOOST_CHECK(ba == std::strong_ordering::greater);
        } else if (ab == std::strong_ordering::greater) {
            BOOST_CHECK(ba == std::strong_ordering::less);
        } else {
            BOOST_ERROR("distinct transactions compared equal");
        }
    };

    for (TxGraph::Ref& a : refs) {
        for (TxGraph::Ref& b : refs) {
            check_pair(a, b);
        }
    }

    BOOST_CHECK(graph->CompareMainOrder(refs[0], refs[1]) == std::strong_ordering::less);
    BOOST_CHECK(graph->CompareMainOrder(refs[1], refs[0]) == std::strong_ordering::greater);
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_ancdesc_union_groups_same_cluster_queries)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(5);
    for (const FeePerWeight& feerate : {FeePerWeight{10, 10}, FeePerWeight{7, 10}, FeePerWeight{6, 10}}) {
        graph->AddTransaction(refs.emplace_back(), feerate);
    }
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[2]);

    const auto check_refs = [](const std::vector<TxGraph::Ref*>& refs, std::set<TxGraph::Ref*> expected) {
        const std::set<TxGraph::Ref*> unique_refs{refs.begin(), refs.end()};
        BOOST_CHECK_EQUAL(refs.size(), expected.size());
        BOOST_CHECK(unique_refs == expected);
    };

    std::vector<TxGraph::Ref*> sibling_query{&refs[1], &refs[2]};
    check_refs(graph->GetAncestorsUnion(sibling_query, TxGraph::Level::TOP),
               {&refs[0], &refs[1], &refs[2]});
    check_refs(graph->GetDescendantsUnion(sibling_query, TxGraph::Level::TOP),
               {&refs[1], &refs[2]});

    graph->StartStaging();
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{5, 10});
    graph->AddDependency(/*parent=*/refs[1], /*child=*/refs[3]);

    std::vector<TxGraph::Ref*> staged_query{&refs[3], &refs[2]};
    check_refs(graph->GetAncestorsUnion(staged_query, TxGraph::Level::TOP),
               {&refs[0], &refs[1], &refs[2], &refs[3]});
    check_refs(graph->GetDescendantsUnion(sibling_query, TxGraph::Level::TOP),
               {&refs[1], &refs[2], &refs[3]});
    check_refs(graph->GetAncestorsUnion(staged_query, TxGraph::Level::MAIN),
               {&refs[0], &refs[2]});

    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_ref_move_preserves_removed_entry)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    TxGraph::Ref parent;
    TxGraph::Ref child;
    graph->AddTransaction(parent, FeePerWeight{10, 10});
    graph->AddTransaction(child, FeePerWeight{1, 10});
    graph->AddDependency(/*parent=*/parent, /*child=*/child);

    BOOST_CHECK(graph->Exists(child, TxGraph::Level::TOP));
    graph->RemoveTransaction(child);
    BOOST_CHECK(!graph->Exists(child, TxGraph::Level::TOP));
    graph->SanityCheck();

    {
        TxGraph::Ref moved_child{std::move(child)};
        BOOST_CHECK(!graph->Exists(child, TxGraph::Level::TOP));
        BOOST_CHECK(!graph->Exists(moved_child, TxGraph::Level::TOP));
        graph->AddDependency(/*parent=*/parent, /*child=*/moved_child);
        BOOST_CHECK(!graph->Exists(moved_child, TxGraph::Level::TOP));
        graph->SanityCheck();
    }

    BOOST_CHECK(graph->Exists(parent, TxGraph::Level::TOP));
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_ref_move_after_graph_destruction_is_empty)
{
    TxGraph::Ref ref;
    {
        auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);
        graph->AddTransaction(ref, FeePerWeight{10, 10});
        BOOST_CHECK(graph->Exists(ref, TxGraph::Level::TOP));
        graph->SanityCheck();
    }

    TxGraph::Ref moved_ref{std::move(ref)};

    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);
    BOOST_CHECK(!graph->Exists(ref, TxGraph::Level::TOP));
    BOOST_CHECK(!graph->Exists(moved_ref, TxGraph::Level::TOP));
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_staging)
{
    /* Create a new graph for the test.
     * The parameters are max_cluster_count, max_cluster_size, acceptable_iters
     */
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(2);

    FeePerWeight feerateA{2, 10};
    FeePerWeight feerateB{1, 10};

    // everytime adding a transaction, test the chunk status
    // [A]
    graph->AddTransaction(refs.emplace_back(), feerateA);
    BOOST_CHECK_EQUAL(graph->HaveStaging(), false);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);

    graph->StartStaging();
    BOOST_CHECK_EQUAL(graph->HaveStaging(), true);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);

    {
        TxGraph::Ref aborted_ref;
        graph->AddTransaction(aborted_ref, feerateB);
        graph->RemoveTransaction(refs[0]);
        BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 1);
        BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);

        graph->AbortStaging();
        BOOST_CHECK_EQUAL(graph->HaveStaging(), false);
        BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 1);
        BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);
        BOOST_CHECK_EQUAL(graph->Exists(refs[0], TxGraph::Level::MAIN), true);
        BOOST_CHECK_EQUAL(graph->Exists(refs[0], TxGraph::Level::TOP), true);
        BOOST_CHECK_EQUAL(graph->Exists(aborted_ref, TxGraph::Level::TOP), false);
        graph->SanityCheck();
    }

    graph->StartStaging();
    BOOST_CHECK_EQUAL(graph->HaveStaging(), true);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);

    // [A, B]
    graph->AddTransaction(refs.emplace_back(), feerateB);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 1);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 2);
    BOOST_CHECK_EQUAL(graph->Exists(refs[0], TxGraph::Level::TOP), true);
    BOOST_CHECK_EQUAL(graph->Exists(refs[1], TxGraph::Level::TOP), true);

    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 1);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 2);

    graph->CommitStaging();
    BOOST_CHECK_EQUAL(graph->HaveStaging(), false);

    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 2);

    graph->StartStaging();

    // [A]
    graph->RemoveTransaction(refs[1]);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 2);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 1);

    graph->CommitStaging();

    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 1);

    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_main_memory_usage_matches_abort_staging)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(4);

    graph->AddTransaction(refs.emplace_back(), FeePerWeight{10, 10});
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{5, 10});
    graph->AddDependency(/*parent=*/refs[0], /*child=*/refs[1]);

    const auto main_usage{graph->GetMainMemoryUsage()};
    BOOST_CHECK_GT(main_usage, 0U);

    graph->StartStaging();
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{4, 10});
    graph->AddDependency(/*parent=*/refs[1], /*child=*/refs[2]);
    graph->RemoveTransaction(refs[0]);

    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 2U);
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::TOP), 2U);
    const auto staged_usage{graph->GetMainMemoryUsage()};
    BOOST_CHECK_GT(staged_usage, 0U);

    graph->AbortStaging();
    BOOST_CHECK_EQUAL(graph->GetMainMemoryUsage(), staged_usage);
    BOOST_CHECK(graph->Exists(refs[0], TxGraph::Level::MAIN));
    BOOST_CHECK(graph->Exists(refs[1], TxGraph::Level::MAIN));
    BOOST_CHECK(!graph->Exists(refs[2], TxGraph::Level::MAIN));
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_main_memory_usage_after_staged_unlink_clears_oversized)
{
    auto graph = MakeTxGraph(2, 268'413'632, 2517, PointerComparator);

    std::vector<std::unique_ptr<TxGraph::Ref>> refs;
    refs.reserve(4);

    auto add_ref = [&]() -> TxGraph::Ref& {
        refs.push_back(std::make_unique<TxGraph::Ref>());
        return *refs.back();
    };

    graph->AddTransaction(add_ref(), FeePerWeight{1, 1});
    graph->AddTransaction(add_ref(), FeePerWeight{1, 167});
    graph->AddDependency(/*parent=*/ *refs[1], /*child=*/ *refs[0]);
    graph->AddTransaction(add_ref(), FeePerWeight{129, 1});
    graph->AddTransaction(add_ref(), FeePerWeight{166, 152});
    graph->AddDependency(/*parent=*/ *refs[1], /*child=*/ *refs[3]);

    graph->StartStaging();
    refs[0].reset();

    const auto staged_usage{graph->GetMainMemoryUsage()};
    BOOST_CHECK_GT(staged_usage, 0U);
    graph->AbortStaging();
    BOOST_CHECK_GT(graph->GetMainMemoryUsage(), 0U);
    BOOST_CHECK(!graph->IsOversized(TxGraph::Level::MAIN));
    BOOST_CHECK_EQUAL(graph->GetTransactionCount(TxGraph::Level::MAIN), 3U);
    BOOST_CHECK(graph->Exists(*refs[1], TxGraph::Level::MAIN));
    BOOST_CHECK(graph->Exists(*refs[2], TxGraph::Level::MAIN));
    BOOST_CHECK(graph->Exists(*refs[3], TxGraph::Level::MAIN));
    graph->SanityCheck();
}

BOOST_AUTO_TEST_CASE(txgraph_staging_diagrams_sort_equal_feerate_chunks)
{
    auto graph = MakeTxGraph(10, 1000, HIGH_ACCEPTABLE_COST, PointerComparator);

    std::vector<TxGraph::Ref> refs;
    refs.reserve(5);

    // Add the larger equal-feerate chunk first. GetMainStagingDiagrams() must sort equal-feerate
    // chunks using the full public diagram order, not preserve insertion order.
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{30, 10});
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{15, 5});
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{7, 7});

    graph->StartStaging();
    graph->RemoveTransaction(refs[0]);
    graph->RemoveTransaction(refs[1]);
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{30, 10});
    graph->AddTransaction(refs.emplace_back(), FeePerWeight{15, 5});

    const auto [main_diagram, staging_diagram] = graph->GetMainStagingDiagrams();
    CheckDiagram(main_diagram, {{15, 5}, {30, 10}});
    CheckDiagram(staging_diagram, {{15, 5}, {30, 10}});

    const FeeFrac main_sum{main_diagram[0] + main_diagram[1]};
    const FeeFrac staging_sum{staging_diagram[0] + staging_diagram[1]};
    BOOST_CHECK(staging_sum == main_sum);
    graph->SanityCheck();
}

BOOST_AUTO_TEST_SUITE_END()
