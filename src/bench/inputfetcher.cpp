// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <coins.h>
#include <common/system.h>
#include <inputfetcher.h>
#include <primitives/block.h>
#include <serialize.h>
#include <streams.h>
#include <util/time.h>

#include <cassert>

//! Simulates a DB by adding a delay when calling GetCoin
struct DelayedCoinsView : CCoinsView
{
    int m_write_count{0};
    std::optional<Coin> GetCoin(const COutPoint&) const override
    {
        UninterruptibleSleep(2ms);
        Coin coin;
        coin.out.nValue = 1;  // Unspent
        return coin;
    }

    bool BatchWrite(CoinsViewCacheCursor& cursor, const uint256&) override
    {
        for (auto it{cursor.Begin()}; it != cursor.End(); it = cursor.NextAndMaybeErase(*it)) {
            m_write_count++;
        }
        return true;
    }
};

static void InputFetcherBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    DelayedCoinsView db{};
    CCoinsViewCache cache{&db};

    // The main thread should be counted to prevent thread oversubscription, and
    // to decrease the variance of benchmark results.
    InputFetcher fetcher{/*max_thread_count*/4};

    bench.run([&] {
        CCoinsViewCache block_cache{&cache};
        fetcher.FetchInputs(cache, block_cache, db, block);
        assert(db.m_write_count == 0 && cache.GetCacheSize() == 0 && block_cache.GetCacheSize() == 4599);
        // assert(block_cache.Flush());
        // assert(db.m_write_count == 0 && cache.GetCacheSize() == 4599 && block_cache.GetCacheSize() == 0);
    });
}

BENCHMARK(InputFetcherBenchmark, benchmark::PriorityLevel::HIGH);
