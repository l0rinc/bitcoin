// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <support/allocators/pool.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <unordered_map>
#include <utility>

template <typename Map>
void BenchFillClearMap(benchmark::Bench& bench, Map& map)
{
    size_t batch_size = 5000;

    // make sure each iteration of the benchmark contains exactly 5000 inserts and one clear.
    // do this at least 10 times so we get reasonable accurate results

    bench.batch(batch_size).minEpochIterations(10).run([&] {
        auto rng = ankerl::nanobench::Rng(1234);
        for (size_t i = 0; i < batch_size; ++i) {
            map[rng()];
        }
        map.clear();
    });
}

static void PoolAllocator_StdUnorderedMap(benchmark::Bench& bench)
{
    auto map = std::unordered_map<uint64_t, uint64_t>();
    BenchFillClearMap(bench, map);
}

static void PoolAllocator_StdMap(benchmark::Bench& bench)
{
    auto map = std::map<uint64_t, uint64_t>();
    BenchFillClearMap(bench, map);
}

static void PoolAllocator_StdMapWithPoolResource(benchmark::Bench& bench)
{
    using Map = std::map<uint64_t,
                         uint64_t,
                         std::less<uint64_t>,
                         PoolAllocator<std::pair<const uint64_t, uint64_t>,
                                       sizeof(std::pair<const uint64_t, uint64_t>) + 4 * sizeof(void*)>>;

    auto pool_resource = Map::allocator_type::ResourceType();
    auto map = Map{Map::key_compare{}, &pool_resource};
    BenchFillClearMap(bench, map);
}

BENCHMARK(PoolAllocator_StdUnorderedMap, benchmark::PriorityLevel::HIGH);
BENCHMARK(PoolAllocator_StdMap, benchmark::PriorityLevel::HIGH);
BENCHMARK(PoolAllocator_StdMapWithPoolResource, benchmark::PriorityLevel::HIGH);
