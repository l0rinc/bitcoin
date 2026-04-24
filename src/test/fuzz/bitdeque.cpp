// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <util/bitdeque.h>

#include <deque>
#include <vector>
#include <test/util/check.h>

namespace {

constexpr int LEN_BITS = 16;
constexpr int RANDDATA_BITS = 20;

using bitdeque_type = bitdeque<128>;

//! Deterministic random vector of bools, for begin/end insertions to draw from.
std::vector<bool> RANDDATA;

void InitRandData()
{
    FastRandomContext ctx(true);
    RANDDATA.clear();
    for (size_t i = 0; i < (1U << RANDDATA_BITS) + (1U << LEN_BITS); ++i) {
        RANDDATA.push_back(ctx.randbool());
    }
}

} // namespace

FUZZ_TARGET(bitdeque, .init = InitRandData)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    FastRandomContext ctx(true);

    size_t maxlen = (1U << provider.ConsumeIntegralInRange<size_t>(0, LEN_BITS)) - 1;
    size_t limitlen = 4 * maxlen;

    std::deque<bool> deq;
    bitdeque_type bitdeq;

    const auto& cdeq = deq;
    const auto& cbitdeq = bitdeq;

    size_t initlen = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
    while (initlen) {
        bool val = ctx.randbool();
        deq.push_back(val);
        bitdeq.push_back(val);
        --initlen;
    }

    const auto iter_limit{maxlen > 6000 ? 90U : 900U};
    LIMITED_WHILE(provider.remaining_bytes() > 0, iter_limit)
    {
        CallOneOf(
            provider,
            [&] {
                // constructor()
                deq = std::deque<bool>{};
                bitdeq = bitdeque_type{};
            },
            [&] {
                // clear()
                deq.clear();
                bitdeq.clear();
            },
            [&] {
                // resize()
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                deq.resize(count);
                bitdeq.resize(count);
            },
            [&] {
                // assign(count, val)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                deq.assign(count, val);
                bitdeq.assign(count, val);
            },
            [&] {
                // constructor(count, val)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                deq = std::deque<bool>(count, val);
                bitdeq = bitdeque_type(count, val);
            },
            [&] {
                // constructor(count)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                deq = std::deque<bool>(count);
                bitdeq = bitdeque_type(count);
            },
            [&] {
                // construct(begin, end)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                deq = std::deque<bool>(rand_begin, rand_end);
                bitdeq = bitdeque_type(rand_begin, rand_end);
            },
            [&] {
                // assign(begin, end)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                deq.assign(rand_begin, rand_end);
                bitdeq.assign(rand_begin, rand_end);
            },
            [&] {
                // construct(initializer_list)
                std::initializer_list<bool> ilist{ctx.randbool(), ctx.randbool(), ctx.randbool(), ctx.randbool(), ctx.randbool()};
                deq = std::deque<bool>(ilist);
                bitdeq = bitdeque_type(ilist);
            },
            [&] {
                // assign(initializer_list)
                std::initializer_list<bool> ilist{ctx.randbool(), ctx.randbool(), ctx.randbool()};
                deq.assign(ilist);
                bitdeq.assign(ilist);
            },
            [&] {
                // operator=(const&)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                const std::deque<bool> deq2(count, val);
                deq = deq2;
                const bitdeque_type bitdeq2(count, val);
                bitdeq = bitdeq2;
            },
            [&] {
                // operator=(&&)
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                bool val = ctx.randbool();
                std::deque<bool> deq2(count, val);
                deq = std::move(deq2);
                bitdeque_type bitdeq2(count, val);
                bitdeq = std::move(bitdeq2);
            },
            [&] {
                // deque swap
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                std::deque<bool> deq2(rand_begin, rand_end);
                bitdeque_type bitdeq2(rand_begin, rand_end);
                using std::swap;
                CHECK(deq.size() == bitdeq.size());
                CHECK(deq2.size() == bitdeq2.size());
                swap(deq, deq2);
                swap(bitdeq, bitdeq2);
                CHECK(deq.size() == bitdeq.size());
                CHECK(deq2.size() == bitdeq2.size());
            },
            [&] {
                // deque.swap
                auto count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                auto rand_end = rand_begin + count;
                std::deque<bool> deq2(rand_begin, rand_end);
                bitdeque_type bitdeq2(rand_begin, rand_end);
                CHECK(deq.size() == bitdeq.size());
                CHECK(deq2.size() == bitdeq2.size());
                deq.swap(deq2);
                bitdeq.swap(bitdeq2);
                CHECK(deq.size() == bitdeq.size());
                CHECK(deq2.size() == bitdeq2.size());
            },
            [&] {
                // operator=(initializer_list)
                std::initializer_list<bool> ilist{ctx.randbool(), ctx.randbool(), ctx.randbool()};
                deq = ilist;
                bitdeq = ilist;
            },
            [&] {
                // iterator arithmetic
                auto pos1 = provider.ConsumeIntegralInRange<long>(0, cdeq.size());
                auto pos2 = provider.ConsumeIntegralInRange<long>(0, cdeq.size());
                auto it = deq.begin() + pos1;
                auto bitit = bitdeq.begin() + pos1;
                if ((size_t)pos1 != cdeq.size()) CHECK(*it == *bitit);
                CHECK(it - deq.begin() == pos1);
                CHECK(bitit - bitdeq.begin() == pos1);
                if (provider.ConsumeBool()) {
                    it += pos2 - pos1;
                    bitit += pos2 - pos1;
                } else {
                    it -= pos1 - pos2;
                    bitit -= pos1 - pos2;
                }
                if ((size_t)pos2 != cdeq.size()) CHECK(*it == *bitit);
                CHECK(deq.end() - it == bitdeq.end() - bitit);
                if (provider.ConsumeBool()) {
                    if ((size_t)pos2 != cdeq.size()) {
                        ++it;
                        ++bitit;
                    }
                } else {
                    if (pos2 != 0) {
                        --it;
                        --bitit;
                    }
                }
                CHECK(deq.end() - it == bitdeq.end() - bitit);
            },
            [&] {
                // begin() and end()
                CHECK(deq.end() - deq.begin() == bitdeq.end() - bitdeq.begin());
            },
            [&] {
                // begin() and end() (const)
                CHECK(cdeq.end() - cdeq.begin() == cbitdeq.end() - cbitdeq.begin());
            },
            [&] {
                // rbegin() and rend()
                CHECK(deq.rend() - deq.rbegin() == bitdeq.rend() - bitdeq.rbegin());
            },
            [&] {
                // rbegin() and rend() (const)
                CHECK(cdeq.rend() - cdeq.rbegin() == cbitdeq.rend() - cbitdeq.rbegin());
            },
            [&] {
                // cbegin() and cend()
                CHECK(cdeq.cend() - cdeq.cbegin() == cbitdeq.cend() - cbitdeq.cbegin());
            },
            [&] {
                // crbegin() and crend()
                CHECK(cdeq.crend() - cdeq.crbegin() == cbitdeq.crend() - cbitdeq.crbegin());
            },
            [&] {
                // size() and maxsize()
                CHECK(cdeq.size() == cbitdeq.size());
                CHECK(cbitdeq.size() <= cbitdeq.max_size());
            },
            [&] {
                // empty
                CHECK(cdeq.empty() == cbitdeq.empty());
            },
            [&] {
                // at (in range) and flip
                if (!cdeq.empty()) {
                    size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    auto& ref = deq.at(pos);
                    auto bitref = bitdeq.at(pos);
                    CHECK(ref == bitref);
                    if (ctx.randbool()) {
                        ref = !ref;
                        bitref.flip();
                    }
                }
            },
            [&] {
                // at (maybe out of range) and bit assign
                size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() + maxlen);
                bool newval = ctx.randbool();
                bool throw_deq{false}, throw_bitdeq{false};
                bool val_deq{false}, val_bitdeq{false};
                try {
                    auto& ref = deq.at(pos);
                    val_deq = ref;
                    ref = newval;
                } catch (const std::out_of_range&) {
                    throw_deq = true;
                }
                try {
                    auto ref = bitdeq.at(pos);
                    val_bitdeq = ref;
                    ref = newval;
                } catch (const std::out_of_range&) {
                    throw_bitdeq = true;
                }
                CHECK(throw_deq == throw_bitdeq);
                CHECK(throw_bitdeq == (pos >= cdeq.size()));
                if (!throw_deq) CHECK(val_deq == val_bitdeq);
            },
            [&] {
                // at (maybe out of range) (const)
                size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() + maxlen);
                bool throw_deq{false}, throw_bitdeq{false};
                bool val_deq{false}, val_bitdeq{false};
                try {
                    auto& ref = cdeq.at(pos);
                    val_deq = ref;
                } catch (const std::out_of_range&) {
                    throw_deq = true;
                }
                try {
                    auto ref = cbitdeq.at(pos);
                    val_bitdeq = ref;
                } catch (const std::out_of_range&) {
                    throw_bitdeq = true;
                }
                CHECK(throw_deq == throw_bitdeq);
                CHECK(throw_bitdeq == (pos >= cdeq.size()));
                if (!throw_deq) CHECK(val_deq == val_bitdeq);
            },
            [&] {
                // operator[]
                if (!cdeq.empty()) {
                    size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    CHECK(deq[pos] == bitdeq[pos]);
                    if (ctx.randbool()) {
                        deq[pos] = !deq[pos];
                        bitdeq[pos].flip();
                    }
                }
            },
            [&] {
                // operator[] const
                if (!cdeq.empty()) {
                    size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    CHECK(deq[pos] == bitdeq[pos]);
                }
            },
            [&] {
                // front()
                if (!cdeq.empty()) {
                    auto& ref = deq.front();
                    auto bitref = bitdeq.front();
                    CHECK(ref == bitref);
                    if (ctx.randbool()) {
                        ref = !ref;
                        bitref = !bitref;
                    }
                }
            },
            [&] {
                // front() const
                if (!cdeq.empty()) {
                    auto& ref = cdeq.front();
                    auto bitref = cbitdeq.front();
                    CHECK(ref == bitref);
                }
            },
            [&] {
                // back() and swap(bool, ref)
                if (!cdeq.empty()) {
                    auto& ref = deq.back();
                    auto bitref = bitdeq.back();
                    CHECK(ref == bitref);
                    if (ctx.randbool()) {
                        ref = !ref;
                        bitref.flip();
                    }
                }
            },
            [&] {
                // back() const
                if (!cdeq.empty()) {
                    const auto& cdeq = deq;
                    const auto& cbitdeq = bitdeq;
                    auto& ref = cdeq.back();
                    auto bitref = cbitdeq.back();
                    CHECK(ref == bitref);
                }
            },
            [&] {
                // push_back()
                if (cdeq.size() < limitlen) {
                    bool val = ctx.randbool();
                    if (cdeq.empty()) {
                        deq.push_back(val);
                        bitdeq.push_back(val);
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        CHECK(ref == bitref);
                        deq.push_back(val);
                        bitdeq.push_back(val);
                        CHECK(ref == bitref); // references are not invalidated
                    }
                }
            },
            [&] {
                // push_front()
                if (cdeq.size() < limitlen) {
                    bool val = ctx.randbool();
                    if (cdeq.empty()) {
                        deq.push_front(val);
                        bitdeq.push_front(val);
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        CHECK(ref == bitref);
                        deq.push_front(val);
                        bitdeq.push_front(val);
                        CHECK(ref == bitref); // references are not invalidated
                    }
                }
            },
            [&] {
                // pop_back()
                if (!cdeq.empty()) {
                    if (cdeq.size() == 1) {
                        deq.pop_back();
                        bitdeq.pop_back();
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 2);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        CHECK(ref == bitref);
                        deq.pop_back();
                        bitdeq.pop_back();
                        CHECK(ref == bitref); // references to other elements are not invalidated
                    }
                }
            },
            [&] {
                // pop_front()
                if (!cdeq.empty()) {
                    if (cdeq.size() == 1) {
                        deq.pop_front();
                        bitdeq.pop_front();
                    } else {
                        size_t pos = provider.ConsumeIntegralInRange<size_t>(1, cdeq.size() - 1);
                        auto& ref = deq[pos];
                        auto bitref = bitdeq[pos];
                        CHECK(ref == bitref);
                        deq.pop_front();
                        bitdeq.pop_front();
                        CHECK(ref == bitref); // references to other elements are not invalidated
                    }
                }
            },
            [&] {
                // erase (in middle, single)
                if (!cdeq.empty()) {
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - 1);
                    size_t after = cdeq.size() - 1 - before;
                    auto it = deq.erase(cdeq.begin() + before);
                    auto bitit = bitdeq.erase(cbitdeq.begin() + before);
                    CHECK(it == cdeq.begin() + before && it == cdeq.end() - after);
                    CHECK(bitit == cbitdeq.begin() + before && bitit == cbitdeq.end() - after);
                }
            },
            [&] {
                // erase (at front, range)
                size_t count = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                auto it = deq.erase(cdeq.begin(), cdeq.begin() + count);
                auto bitit = bitdeq.erase(cbitdeq.begin(), cbitdeq.begin() + count);
                CHECK(it == deq.begin());
                CHECK(bitit == bitdeq.begin());
            },
            [&] {
                // erase (at back, range)
                size_t count = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                auto it = deq.erase(cdeq.end() - count, cdeq.end());
                auto bitit = bitdeq.erase(cbitdeq.end() - count, cbitdeq.end());
                CHECK(it == deq.end());
                CHECK(bitit == bitdeq.end());
            },
            [&] {
                // erase (in middle, range)
                size_t count = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size() - count);
                size_t after = cdeq.size() - count - before;
                auto it = deq.erase(cdeq.begin() + before, cdeq.end() - after);
                auto bitit = bitdeq.erase(cbitdeq.begin() + before, cbitdeq.end() - after);
                CHECK(it == cdeq.begin() + before && it == cdeq.end() - after);
                CHECK(bitit == cbitdeq.begin() + before && bitit == cbitdeq.end() - after);
            },
            [&] {
                // insert/emplace (in middle, single)
                if (cdeq.size() < limitlen) {
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                    bool val = ctx.randbool();
                    bool do_emplace = provider.ConsumeBool();
                    auto it = deq.insert(cdeq.begin() + before, val);
                    auto bitit = do_emplace ? bitdeq.emplace(cbitdeq.begin() + before, val)
                                            : bitdeq.insert(cbitdeq.begin() + before, val);
                    CHECK(it == deq.begin() + before);
                    CHECK(bitit == bitdeq.begin() + before);
                }
            },
            [&] {
                // insert (at front, begin/end)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                    auto rand_end = rand_begin + count;
                    auto it = deq.insert(cdeq.begin(), rand_begin, rand_end);
                    auto bitit = bitdeq.insert(cbitdeq.begin(), rand_begin, rand_end);
                    CHECK(it == cdeq.begin());
                    CHECK(bitit == cbitdeq.begin());
                }
            },
            [&] {
                // insert (at back, begin/end)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                    auto rand_end = rand_begin + count;
                    auto it = deq.insert(cdeq.end(), rand_begin, rand_end);
                    auto bitit = bitdeq.insert(cbitdeq.end(), rand_begin, rand_end);
                    CHECK(it == cdeq.end() - count);
                    CHECK(bitit == cbitdeq.end() - count);
                }
            },
            [&] {
                // insert (in middle, range)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                    bool val = ctx.randbool();
                    auto it = deq.insert(cdeq.begin() + before, count, val);
                    auto bitit = bitdeq.insert(cbitdeq.begin() + before, count, val);
                    CHECK(it == deq.begin() + before);
                    CHECK(bitit == bitdeq.begin() + before);
                }
            },
            [&] {
                // insert (in middle, begin/end)
                if (cdeq.size() < limitlen) {
                    size_t count = provider.ConsumeIntegralInRange<size_t>(0, maxlen);
                    size_t before = provider.ConsumeIntegralInRange<size_t>(0, cdeq.size());
                    auto rand_begin = RANDDATA.begin() + ctx.randbits(RANDDATA_BITS);
                    auto rand_end = rand_begin + count;
                    auto it = deq.insert(cdeq.begin() + before, rand_begin, rand_end);
                    auto bitit = bitdeq.insert(cbitdeq.begin() + before, rand_begin, rand_end);
                    CHECK(it == deq.begin() + before);
                    CHECK(bitit == bitdeq.begin() + before);
                }
            });
    }
    {
        CHECK(deq.size() == bitdeq.size());
        auto it = deq.begin();
        auto bitit = bitdeq.begin();
        auto itend = deq.end();
        while (it != itend) {
            CHECK(*it == *bitit);
            ++it;
            ++bitit;
        }
    }
}
