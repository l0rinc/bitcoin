// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/nanobench.h>
#include <primitives/transaction_identifier.h>
#include <random.h>
#include <util/check.h>
#include <util/hasher.h>

#include <algorithm>
#include <ranges>
#include <set>
#include <unordered_set>
#include <vector>

namespace {

constexpr size_t iterations{1'000};
constexpr size_t set_size{2'000};

struct Dataset {
    std::set<Txid> sorted_set;
    std::unordered_set<Txid, SaltedTxidHasher> unsorted_set;
    std::vector<Txid> vec_sorted;
    std::vector<Txid> vec_unsorted;

    std::vector<Txid> hits;
    std::vector<Txid> misses;
};

std::vector<Dataset> BuildDatasets()
{
    FastRandomContext rng(/*fDeterministic=*/true);

    std::vector<Dataset> datasets;
    datasets.reserve(iterations);

    // since the inputs of the benchmarks are mutated by sorting, we can't rerun the benchmarks
    for (size_t d{0}; d < iterations; ++d) {
        Dataset ds;
        ds.unsorted_set.reserve(set_size);
        ds.vec_sorted.reserve(set_size);
        ds.vec_unsorted.reserve(set_size);
        ds.hits.reserve(set_size);
        ds.misses.reserve(set_size);

        for (size_t i{0}; i < set_size; ++i) {
            Txid t{Txid::FromUint256(rng.rand256())};
            ds.sorted_set.emplace(t);
        }

        ds.vec_sorted.assign(ds.sorted_set.begin(), ds.sorted_set.end());
        ds.vec_unsorted = ds.vec_sorted;
        for (const auto& t : ds.vec_sorted) {
            ds.unsorted_set.emplace(t);
        }
        std::sort(ds.vec_sorted.begin(), ds.vec_sorted.end());

        for (const auto& t : ds.vec_sorted) {
            ds.hits.emplace_back(t);
            ds.misses.emplace_back(Txid::FromUint256(rng.rand256()));
        }
        std::ranges::shuffle(ds.hits, rng);

        datasets.emplace_back(std::move(ds));
    }
    return datasets;
}

} // namespace

static void Txid_UnorderedSalted(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    bench.name("unordered_salted/hits").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.hits) {
                sum += s.unsorted_set.contains(q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * set_size);
    });
    bench.name("unordered_salted/misses").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.misses) {
                sum += s.unsorted_set.contains(q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == 0);
    });
}

static void Txid_SetOrdered(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    bench.name("set_ordered/hits").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.hits) {
                sum += s.sorted_set.contains(q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * set_size);
    });
    bench.name("set_ordered/misses").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.misses) {
                sum += s.sorted_set.contains(q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == 0);
    });
}

static void Txid_VectorBinarySearch(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    bench.name("vector_binary_search/hits").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.hits) {
                sum += std::binary_search(s.vec_sorted.begin(), s.vec_sorted.end(), q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * set_size);
    });
    bench.name("vector_binary_search/misses").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.misses) {
                sum += std::binary_search(s.vec_sorted.begin(), s.vec_sorted.end(), q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == 0);
    });
}

static void Txid_VectorLinearScan(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    const auto contains_linear{[](const std::vector<Txid>& v, const Txid& x) noexcept {
        return std::ranges::find(v, x) != v.end();
    }};
    bench.name("vector_linear_scan/hits").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.hits) {
                sum += contains_linear(s.vec_unsorted, q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * set_size);
    });
    bench.name("vector_linear_scan/misses").epochs(1).epochIterations(1).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.misses) {
                sum += contains_linear(s.vec_unsorted, q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == 0);
    });
}

BENCHMARK(Txid_UnorderedSalted, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_SetOrdered, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorBinarySearch, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorLinearScan, benchmark::PriorityLevel::LOW);
