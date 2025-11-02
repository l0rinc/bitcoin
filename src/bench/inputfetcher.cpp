// Copyright (c) The Bitcoin Core developers
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

static constexpr auto DELAY{2ms};

//! Simulates a DB by adding a delay when calling GetCoin
struct DelayedCoinsView : CCoinsView {
    std::optional<Coin> GetCoin(const COutPoint&) const override
    {
        UninterruptibleSleep(DELAY);
        Coin coin{};
        coin.out.nValue = 1;
        return coin;
    }
};

static void InputFetcherBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    DelayedCoinsView db{};
    CCoinsViewCache main_cache(&db);

    // The main thread should be counted to prevent thread oversubscription, and
    // to decrease the variance of benchmark results.
    const auto worker_threads_num{GetNumCores() - 1};
    InputFetcher fetcher{worker_threads_num};

    bench.run([&] {
        CCoinsViewCache temp_cache(&main_cache);
        fetcher.FetchInputs(temp_cache, main_cache, db, block);
        ankerl::nanobench::doNotOptimizeAway(&temp_cache);
        Assert(temp_cache.GetCacheSize() == 4599);
    });
}

BENCHMARK(InputFetcherBenchmark, benchmark::PriorityLevel::HIGH);
