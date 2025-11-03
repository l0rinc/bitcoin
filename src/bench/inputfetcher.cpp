// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <coins.h>
#include <inputfetcher.h>
#include <primitives/block.h>
#include <serialize.h>
#include <streams.h>

static void InputFetcher_SortedVectorBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    bench.run([&] {
        std::vector<Txid> v{};
        v.reserve(block.vtx.size());
        for (const auto& tx : block.vtx) {
            v.emplace_back(tx->GetHash());
        }
        std::sort(v.begin(), v.end());
        ankerl::nanobench::doNotOptimizeAway(v);
    });
}

static void InputFetcher_UnorderedSetBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    bench.run([&] {
        std::unordered_set<Txid, SaltedTxidHasher> u{};
        u.reserve(block.vtx.size());
        for (const auto& tx : block.vtx) {
            u.emplace(tx->GetHash());
        }
        ankerl::nanobench::doNotOptimizeAway(u);
    });
}

static void InputFetcher_SetBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    bench.run([&] {
        std::set<Txid> s{};
        for (const auto& tx : block.vtx) {
            s.insert(tx->GetHash());
        }
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

BENCHMARK(InputFetcher_SortedVectorBenchmark, benchmark::PriorityLevel::HIGH);
BENCHMARK(InputFetcher_UnorderedSetBenchmark, benchmark::PriorityLevel::HIGH);
BENCHMARK(InputFetcher_SetBenchmark, benchmark::PriorityLevel::HIGH);
