// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <leveldb/comparator.h>
#include <leveldb/db/dbformat.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/table/block.h>
#include <leveldb/table/block_builder.h>
#include <leveldb/table/format.h>
#include <leveldb/table/merger.h>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

class VectorIterator : public leveldb::Iterator {
public:
    using Entry = std::pair<std::string, std::string>;

    explicit VectorIterator(std::initializer_list<Entry> entries)
        : m_entries{entries}, m_index{m_entries.size()}
    {
    }

    bool Valid() const override { return m_index < m_entries.size(); }

    void SeekToFirst() override { m_index = 0; }

    void SeekToLast() override
    {
        m_index = m_entries.empty() ? m_entries.size() : m_entries.size() - 1;
    }

    void Seek(const leveldb::Slice& target) override
    {
        const std::string key{target.ToString()};
        m_index = std::lower_bound(m_entries.begin(), m_entries.end(), key,
                                   [](const Entry& entry, const std::string& key) {
                                       return entry.first < key;
                                   }) -
                  m_entries.begin();
    }

    void Next() override
    {
        assert(Valid());
        ++m_index;
    }

    void Prev() override
    {
        assert(Valid());
        if (m_index == 0) {
            m_index = m_entries.size();
        } else {
            --m_index;
        }
    }

    leveldb::Slice key() const override
    {
        assert(Valid());
        return m_entries[m_index].first;
    }

    leveldb::Slice value() const override
    {
        assert(Valid());
        return m_entries[m_index].second;
    }

    leveldb::Status status() const override { return leveldb::Status::OK(); }

private:
    const std::vector<Entry> m_entries;
    size_t m_index;
};

std::string ReadForward(leveldb::Iterator& iterator)
{
    std::string result;
    for (iterator.SeekToFirst(); iterator.Valid(); iterator.Next()) {
        if (!result.empty()) result += ",";
        result += iterator.key().ToString();
        result += ":";
        result += iterator.value().ToString();
    }
    BOOST_CHECK(iterator.status().ok());
    return result;
}

std::string ReadBackward(leveldb::Iterator& iterator)
{
    std::string result;
    for (iterator.SeekToLast(); iterator.Valid(); iterator.Prev()) {
        if (!result.empty()) result += ",";
        result += iterator.key().ToString();
        result += ":";
        result += iterator.value().ToString();
    }
    BOOST_CHECK(iterator.status().ok());
    return result;
}

class ReverseComparator final : public leveldb::Comparator {
public:
    const char* Name() const override { return "bitcoin.ReverseComparator"; }

    int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override
    {
        return leveldb::BytewiseComparator()->Compare(b, a);
    }

    void FindShortestSeparator(std::string*, const leveldb::Slice&) const override {}
    void FindShortSuccessor(std::string*) const override {}
};

} // namespace

BOOST_AUTO_TEST_SUITE(leveldb_tests)

BOOST_AUTO_TEST_CASE(merging_iterator_preserves_duplicate_tie_order)
{
    leveldb::Iterator* children[] = {
        new VectorIterator{{"a", "zero"}, {"c", "zero"}, {"e", "zero"}},
        new VectorIterator{{"a", "one"}, {"b", "one"}, {"e", "one"}},
        new VectorIterator{{"a", "two"}, {"c", "two"}, {"d", "two"}},
    };
    const std::unique_ptr<leveldb::Iterator> iterator{
        leveldb::NewMergingIterator(leveldb::BytewiseComparator(), children, 3)};

    BOOST_CHECK_EQUAL(ReadForward(*iterator), "a:zero,a:one,a:two,b:one,c:zero,c:two,d:two,e:zero,e:one");
    BOOST_CHECK_EQUAL(ReadBackward(*iterator), "e:one,e:zero,d:two,c:two,c:zero,b:one,a:two,a:one,a:zero");

    iterator->Seek("c");
    BOOST_REQUIRE(iterator->Valid());
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "c");
    BOOST_CHECK_EQUAL(iterator->value().ToString(), "zero");
    iterator->Next();
    BOOST_REQUIRE(iterator->Valid());
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "c");
    BOOST_CHECK_EQUAL(iterator->value().ToString(), "two");
}

BOOST_AUTO_TEST_CASE(merging_iterator_changes_direction_without_skipping_entries)
{
    leveldb::Iterator* children[] = {
        new VectorIterator{{"a", "zero"}, {"d", "zero"}},
        new VectorIterator{{"b", "one"}, {"e", "one"}},
        new VectorIterator{{"c", "two"}, {"f", "two"}},
    };
    const std::unique_ptr<leveldb::Iterator> iterator{
        leveldb::NewMergingIterator(leveldb::BytewiseComparator(), children, 3)};

    iterator->SeekToFirst();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "a");
    iterator->Next();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "b");
    iterator->Next();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "c");
    iterator->Prev();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "b");
    iterator->Next();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "c");

    iterator->SeekToLast();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "f");
    iterator->Prev();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "e");
    iterator->Prev();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "d");
    iterator->Next();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "e");
}

BOOST_AUTO_TEST_CASE(block_iterator_crosses_restart_points_in_both_directions)
{
    leveldb::Options options;
    options.block_restart_interval = 2;
    leveldb::BlockBuilder builder{&options};
    for (int i = 0; i < 8; ++i) {
        const std::string key{"key" + std::to_string(i)};
        builder.Add(key, "value" + std::to_string(i));
    }

    const leveldb::Slice built{builder.Finish()};
    auto data{std::make_unique<char[]>(built.size())};
    std::memcpy(data.get(), built.data(), built.size());
    leveldb::BlockContents contents{
        .data = leveldb::Slice{data.release(), built.size()},
        .cachable = false,
        .heap_allocated = true,
    };
    leveldb::Block block{contents};
    const std::unique_ptr<leveldb::Iterator> iterator{block.NewIterator(leveldb::BytewiseComparator())};

    BOOST_CHECK_EQUAL(ReadForward(*iterator), "key0:value0,key1:value1,key2:value2,key3:value3,key4:value4,key5:value5,key6:value6,key7:value7");
    BOOST_CHECK_EQUAL(ReadBackward(*iterator), "key7:value7,key6:value6,key5:value5,key4:value4,key3:value3,key2:value2,key1:value1,key0:value0");

    iterator->Seek("key4");
    BOOST_REQUIRE(iterator->Valid());
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "key4");
    iterator->Prev();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "key3");
    iterator->Next();
    BOOST_CHECK_EQUAL(iterator->key().ToString(), "key4");
}

BOOST_AUTO_TEST_CASE(internal_key_comparator_respects_non_bytewise_comparator)
{
    ReverseComparator user_comparator;
    leveldb::InternalKeyComparator comparator{&user_comparator};
    const leveldb::InternalKey b{"b", /*sequence=*/1, leveldb::kTypeValue};
    const leveldb::InternalKey a{"a", /*sequence=*/1, leveldb::kTypeValue};

    BOOST_CHECK_LT(comparator.Compare(b.Encode(), a.Encode()), 0);
}

BOOST_AUTO_TEST_SUITE_END()
