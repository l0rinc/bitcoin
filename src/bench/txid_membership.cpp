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

constexpr size_t iterations{1'000}; // since the inputs of the benchmarks are mutated by sorting, we can't rerun the benchmarks
constexpr size_t hits_count{275};   // assuming ~5% of blocks contain internal spends
constexpr size_t tx_count{5500};

struct Dataset {
    std::set<Txid> sorted_set;
    std::unordered_set<Txid, SaltedTxidHasher> unsorted_set;
    std::vector<Txid> vec_sorted;
    std::vector<Txid> vec_unsorted;

    std::vector<Txid> queries;
};

std::vector<Dataset> BuildDatasets()
{
    FastRandomContext rng(/*fDeterministic=*/true);

    std::vector<Dataset> datasets;
    datasets.reserve(iterations);

    for (size_t d{0}; d < iterations; ++d) {
        Dataset ds;
        ds.queries.reserve(tx_count);
        ds.unsorted_set.reserve(tx_count);
        ds.vec_sorted.reserve(tx_count);
        ds.vec_unsorted.reserve(tx_count);

        for (size_t i{0}; i < tx_count; ++i) {
            Txid t{Txid::FromUint256(rng.rand256())};
            ds.sorted_set.emplace(t);
            ds.unsorted_set.emplace(t);
            ds.vec_sorted.emplace_back(t);
            ds.vec_unsorted.emplace_back(t);

            ds.queries.emplace_back(i < hits_count ? t : Txid::FromUint256(rng.rand256()));
        }

        std::ranges::shuffle(ds.queries, rng);
        std::ranges::shuffle(ds.vec_unsorted, rng);
        std::sort(ds.vec_sorted.begin(), ds.vec_sorted.end());

        datasets.emplace_back(std::move(ds));
    }
    return datasets;
}

} // namespace

static void Txid_UnorderedSalted(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    bench.epochs(1).epochIterations(1).batch(iterations).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.queries) {
                sum += s.unsorted_set.contains(q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * hits_count);
    });
}

static void Txid_SetOrdered(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    bench.epochs(1).epochIterations(1).batch(iterations).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.queries) {
                sum += s.sorted_set.contains(q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * hits_count);
    });
}

static void Txid_VectorBinarySearch(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    bench.epochs(1).epochIterations(1).batch(iterations).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.queries) {
                sum += std::binary_search(s.vec_sorted.begin(), s.vec_sorted.end(), q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * hits_count);
    });
}

static void Txid_VectorLinearScan(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets()};
    const auto contains_linear{[](const std::vector<Txid>& v, const Txid& x) noexcept {
        return std::ranges::find(v, x) != v.end();
    }};
    bench.epochs(1).epochIterations(1).batch(iterations).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.queries) {
                sum += contains_linear(s.vec_unsorted, q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * hits_count);
    });
}

BENCHMARK(Txid_UnorderedSalted, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_SetOrdered, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorBinarySearch, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorLinearScan, benchmark::PriorityLevel::LOW);
