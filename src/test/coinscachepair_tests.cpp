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
    CoinsCachePair* head{&sentinel};
    for (auto i{0}; i < NUM_NODES; ++i) {
        nodes.emplace_back();

        auto node{std::prev(nodes.end())};
        CCoinsCacheEntry::SetDirty(*node, sentinel);

        BOOST_CHECK(node->second.IsDirty() && !node->second.IsFresh());
        BOOST_CHECK_EQUAL(node->second.Next(), head);
        head = &(*node);
        BOOST_CHECK_EQUAL(sentinel.second.Next(), head);
    }
    return nodes;
}

BOOST_AUTO_TEST_CASE(linked_list_iteration)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    // Check iterating through pairs is identical to iterating through a list
    auto node{sentinel.second.Next()};
    for (auto expected{nodes.rbegin()}; expected != nodes.rend(); ++expected) {
        BOOST_CHECK_EQUAL(&(*expected), node);
        node = node->second.Next();
    }
    BOOST_CHECK_EQUAL(node, &sentinel);

    // Clear the state during iteration (always unlinking the list head).
    CoinsCachePair* prev{&sentinel};
    node = sentinel.second.Next();
    for (auto expected{nodes.rbegin()}; expected != nodes.rend(); ++expected) {
        BOOST_CHECK_EQUAL(&(*expected), node);
        auto next = node->second.Next();
        CCoinsCacheEntry::SetClean(*prev, *node);
        node = next;
    }
    BOOST_CHECK_EQUAL(node, &sentinel);
    // Check that sentinel's next is itself
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);

    // Delete the nodes from the list to make sure there are no dangling pointers.
    for (auto it{nodes.begin()}; it != nodes.end(); it = nodes.erase(it)) {
        BOOST_CHECK(!it->second.IsDirty() && !it->second.IsFresh());
    }
}

BOOST_AUTO_TEST_CASE(linked_list_random_unlink)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    CoinsCachePair n1;
    CoinsCachePair n2;
    CoinsCachePair n3;
    CoinsCachePair n4;

    // Create linked list sentinel->n4->n3->n2->n1->sentinel.
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    CCoinsCacheEntry::SetDirty(n2, sentinel);
    CCoinsCacheEntry::SetDirty(n3, sentinel);
    CCoinsCacheEntry::SetDirty(n4, sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n4);
    BOOST_CHECK_EQUAL(n4.second.Next(), &n3);
    BOOST_CHECK_EQUAL(n3.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);

    // Unlink n2 (interior node).
    CCoinsCacheEntry::SetClean(n3, n2);
    BOOST_CHECK(!n2.second.IsDirty() && !n2.second.IsFresh());
    BOOST_CHECK_EQUAL(n3.second.Next(), &n1);

    // Unlink the head (n4).
    CCoinsCacheEntry::SetClean(sentinel, n4);
    BOOST_CHECK(!n4.second.IsDirty() && !n4.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n3);

    // Unlink the remaining nodes in order.
    CCoinsCacheEntry::SetClean(sentinel, n3);
    CCoinsCacheEntry::SetClean(sentinel, n1);
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

    // Check that setting DIRTY and FRESH on new node inserts it at the head
    CCoinsCacheEntry::SetDirty(n2, sentinel, /*fresh=*/true);
    BOOST_CHECK(n2.second.IsFresh() && n2.second.IsDirty());
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);

    // Check that we can set extra state, but they don't change our position
    CCoinsCacheEntry::SetDirty(n1, sentinel, /*fresh=*/true);
    BOOST_CHECK(n1.second.IsDirty() && n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);

    // Check that we can clear state then re-set it
    CCoinsCacheEntry::SetClean(n2, n1);
    BOOST_CHECK(!n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);

    // Calling `SetClean` a second time has no effect
    CCoinsCacheEntry::SetClean(n2, n1);
    BOOST_CHECK(!n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);

    // Adding DIRTY re-inserts it at the head
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);
}

BOOST_AUTO_TEST_SUITE_END()
