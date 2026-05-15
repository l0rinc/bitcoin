// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <boost/test/unit_test.hpp>

#include <list>

BOOST_AUTO_TEST_SUITE(coinscachepair_tests)

static constexpr auto NUM_NODES{4};

std::list<CoinsCachePair> CreatePairs(CoinsCachePair& sentinel)
{
    std::list<CoinsCachePair> nodes;
    for (auto i{0}; i < NUM_NODES; ++i) {
        nodes.emplace_back();

        auto node{std::prev(nodes.end())};
        CCoinsCacheEntry::SetDirty(*node, sentinel);

        BOOST_CHECK(node->second.IsDirty() && !node->second.IsFresh());
        BOOST_CHECK_EQUAL(sentinel.second.Next(), &(*node));
    }
    return nodes;
}

BOOST_AUTO_TEST_CASE(linked_list_iteration)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    auto node{sentinel.second.Next()};
    for (auto expected{nodes.rbegin()}; expected != nodes.rend(); ++expected) {
        BOOST_CHECK_EQUAL(&(*expected), node);
        node = node->second.Next();
    }
    BOOST_CHECK_EQUAL(node, &sentinel);
}

BOOST_AUTO_TEST_CASE(linked_list_iterate_unlink)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    auto previous{&sentinel};
    auto node{sentinel.second.Next()};
    for (auto expected{nodes.rbegin()}; expected != nodes.rend(); ++expected) {
        BOOST_CHECK_EQUAL(&(*expected), node);
        auto next{node->second.Next()};
        CCoinsCacheEntry::UnlinkAfter(*previous, *node);
        BOOST_CHECK(!node->second.IsDirty() && !node->second.IsFresh());
        node = next;
    }
    BOOST_CHECK_EQUAL(node, &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);
}

BOOST_AUTO_TEST_CASE(linked_list_set_state)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    CoinsCachePair n1;
    CoinsCachePair n2;

    // Check that setting DIRTY inserts it into linked list and sets state
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n1);

    // Check that setting FRESH on new node inserts it before n1
    CCoinsCacheEntry::SetFresh(n2, sentinel);
    BOOST_CHECK(n2.second.IsFresh() && !n2.second.IsDirty());
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);

    // Check that we can set extra state, but it doesn't change position
    CCoinsCacheEntry::SetFresh(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);

    // Check that we can clear state after unlinking, then re-set it
    CCoinsCacheEntry::UnlinkAfter(n2, n1);
    BOOST_CHECK(!n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);

    // Adding DIRTY re-inserts it before n2
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &n2);
}

BOOST_AUTO_TEST_SUITE_END()
