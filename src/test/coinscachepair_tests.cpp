// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>

#include <boost/test/unit_test.hpp>

#include <list>
#include <vector>

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

    std::vector<CoinsCachePair*> expected;
    for (auto it{nodes.rbegin()}; it != nodes.rend(); ++it) {
        expected.push_back(&(*it));
    }

    size_t idx{0};
    for (auto node{sentinel.second.Next()}; node != &sentinel; node = node->second.Next()) {
        BOOST_CHECK_EQUAL(expected.at(idx++), node);
    }
    BOOST_CHECK_EQUAL(idx, expected.size());

    CoinsCachePair* prev{&sentinel};
    for (auto node{sentinel.second.Next()}; node != &sentinel; ) {
        auto next{node->second.Next()};
        node->second.SetClean(*prev);
        node = next;
    }
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);
}

BOOST_AUTO_TEST_CASE(linked_list_middle_removal)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    auto nodes{CreatePairs(sentinel)};

    std::vector<CoinsCachePair*> order;
    for (auto it{nodes.rbegin()}; it != nodes.rend(); ++it) {
        order.push_back(&(*it));
    }

    auto head{order.front()};
    auto middle{order.at(1)};
    auto tail_after_middle{order.at(2)};

    middle->second.SetClean(*head);
    BOOST_CHECK_EQUAL(head->second.Next(), tail_after_middle);

    head->second.SetClean(sentinel);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), tail_after_middle);

    CoinsCachePair* prev{&sentinel};
    for (auto node{sentinel.second.Next()}; node != &sentinel; ) {
        auto next{node->second.Next()};
        node->second.SetClean(*prev);
        node = next;
    }
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

    // Check that we can set extra state without changing position
    CCoinsCacheEntry::SetFresh(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &n1);

    // Clear n1 and ensure n2 remains linked
    n1.second.SetClean(n2);
    BOOST_CHECK(!n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n2);
    BOOST_CHECK_EQUAL(n2.second.Next(), &sentinel);

    // Clear n2 and ensure sentinel points to itself
    n2.second.SetClean(sentinel);
    BOOST_CHECK(!n2.second.IsDirty() && !n2.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);

    // Re-adding DIRTY re-inserts it at the head
    CCoinsCacheEntry::SetDirty(n1, sentinel);
    BOOST_CHECK(n1.second.IsDirty() && !n1.second.IsFresh());
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &n1);
    BOOST_CHECK_EQUAL(n1.second.Next(), &sentinel);
}

BOOST_AUTO_TEST_CASE(cursor_removes_pruned_entry)
{
    CoinsCachePair sentinel;
    sentinel.second.SelfRef(sentinel);
    CCoinsMapMemoryResource resource;
    CCoinsMap map{0, CCoinsMap::hasher{}, CCoinsMap::key_equal{}, &resource};

    auto [it, inserted] = map.emplace();
    BOOST_REQUIRE(inserted);
    it->second.coin.Clear(); // spent
    CCoinsCacheEntry::SetDirty(*it, sentinel);
    CCoinsCacheEntry::SetFresh(*it, sentinel);
    size_t pruned{1};

    CoinsViewCacheCursor cursor{sentinel, map, /*will_erase=*/false, &pruned};
    auto node{cursor.Begin()};
    BOOST_REQUIRE(node != cursor.End());
    BOOST_CHECK_EQUAL(pruned, 1);

    auto end{cursor.NextAndMaybeErase(*node)};
    BOOST_CHECK_EQUAL(end, cursor.End());
    BOOST_CHECK(map.empty());
    BOOST_CHECK_EQUAL(pruned, 0);
    BOOST_CHECK_EQUAL(sentinel.second.Next(), &sentinel);
}

BOOST_AUTO_TEST_SUITE_END()
