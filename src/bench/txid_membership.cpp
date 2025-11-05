// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <bench/nanobench.h>
#include <coins.h>
#include <functional>
#include <inputfetcher.h>
#include <primitives/block.h>
#include <primitives/transaction_identifier.h>
#include <random.h>
#include <ranges>
#include <serialize.h>
#include <set>
#include <streams.h>
#include <unordered_set>
#include <util/check.h>
#include <util/hasher.h>
#include <vector>

namespace {
constexpr size_t iterations{100}; // since the inputs of the benchmarks are mutated by sorting, we can't rerun the benchmarks
constexpr size_t hits_count{275}; // assuming ~5% of blocks contain internal spends
constexpr size_t tx_count{5500};

uint64_t GetShortID(const Txid& txid)
{
    return txid.ToUint256().GetUint64(0);
}

template <typename T>
struct Dataset {
    std::set<T> sorted_set;
    std::unordered_set<T, std::conditional_t<std::is_same_v<T, Txid>, SaltedTxidHasher, std::identity>> unsorted_set;
    std::vector<T> vec_sorted;
    std::vector<T> vec_unsorted;
    std::vector<T> queries;

    static T Convert(const Txid& txid)
    {
        if constexpr (std::is_same_v<T, Txid>) {
            return txid;
        } else {
            static_assert(std::is_same_v<T, uint64_t>);
            return GetShortID(txid);
        }
    }
};

template <typename T>
std::vector<Dataset<T>> BuildDatasets()
{
    FastRandomContext rng(/*fDeterministic=*/true);

    std::vector<Dataset<T>> datasets;
    datasets.reserve(iterations);

    for (size_t d{0}; d < iterations; ++d) {
        Dataset<T> ds;
        ds.queries.reserve(tx_count);
        ds.unsorted_set.reserve(tx_count);
        ds.vec_sorted.reserve(tx_count);
        ds.vec_unsorted.reserve(tx_count);

        for (size_t i{0}; i < tx_count; ++i) {
            T t1{Dataset<T>::Convert(Txid::FromUint256(rng.rand256()))};
            ds.sorted_set.emplace(t1);
            ds.unsorted_set.emplace(t1);
            ds.vec_sorted.emplace_back(t1);
            ds.vec_unsorted.emplace_back(t1);

            T t2{Dataset<T>::Convert(Txid::FromUint256(rng.rand256()))};
            ds.queries.emplace_back(i < hits_count ? t1 : t2);
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
    static auto ds{BuildDatasets<Txid>()};
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
    static auto ds{BuildDatasets<Txid>()};
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
    static auto ds{BuildDatasets<Txid>()};
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
    static auto ds{BuildDatasets<Txid>()};
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

static void Txid_UnorderedSalted_shortid(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets<uint64_t>()};
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

static void Txid_SetOrdered_shortid(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets<uint64_t>()};
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

static void Txid_VectorBinarySearch_shortid(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets<uint64_t>()};
    bench.epochs(1).epochIterations(1).batch(iterations).run([&] {
        size_t sum{0};
        for (const auto& s : ds) {
            for (const auto& q : s.queries) {
                sum += std::ranges::binary_search(s.vec_sorted, q);
            }
        }
        ankerl::nanobench::doNotOptimizeAway(sum);
        Assert(sum == iterations * hits_count);
    });
}

static void Txid_VectorLinearScan_shortid(benchmark::Bench& bench)
{
    static auto ds{BuildDatasets<uint64_t>()};
    const auto contains_linear{[](const std::vector<uint64_t>& v, uint64_t x) noexcept {
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

static void InputFetcher_SortedVectorBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    std::vector<Txid> v{};
    v.reserve(block.vtx.size());
    bench.run([&] {
        for (const auto& tx : block.vtx) {
            v.emplace_back(tx->GetHash());
        }
        std::sort(v.begin(), v.end());
        ankerl::nanobench::doNotOptimizeAway(v);
        v.clear();
    });
}

static void InputFetcher_UnorderedSetBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    std::unordered_set<Txid, SaltedTxidHasher> u{};
    u.reserve(block.vtx.size());
    bench.run([&] {
        for (const auto& tx : block.vtx) {
            u.emplace(tx->GetHash());
        }
        ankerl::nanobench::doNotOptimizeAway(u);
        u.clear();
    });
}

static void InputFetcher_SetBenchmark(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    std::set<Txid> s{};
    bench.run([&] {
        for (const auto& tx : block.vtx) {
            s.insert(tx->GetHash());
        }
        ankerl::nanobench::doNotOptimizeAway(s);
        s.clear();
    });
}

static void InputFetcher_SortedVectorBenchmark_shortid(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    std::vector<uint64_t> v{};
    v.reserve(block.vtx.size());
    bench.run([&] {
        for (const auto& tx : block.vtx) {
            v.emplace_back(GetShortID(tx->GetHash()));
        }
        std::ranges::sort(v);
        ankerl::nanobench::doNotOptimizeAway(v);
        v.clear();
    });
}

static void InputFetcher_UnorderedSetBenchmark_shortid(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    std::unordered_set<uint64_t> u{};
    u.reserve(block.vtx.size());
    bench.run([&] {
        for (const auto& tx : block.vtx) {
            u.emplace(GetShortID(tx->GetHash()));
        }
        ankerl::nanobench::doNotOptimizeAway(u);
        u.clear();
    });
}

static void InputFetcher_SetBenchmark_shortid(benchmark::Bench& bench)
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);

    std::set<uint64_t> s{};
    bench.run([&] {
        for (const auto& tx : block.vtx) {
            s.insert(GetShortID(tx->GetHash()));
        }
        ankerl::nanobench::doNotOptimizeAway(s);
        s.clear();
    });
}

BENCHMARK(InputFetcher_SortedVectorBenchmark, benchmark::PriorityLevel::LOW);
BENCHMARK(InputFetcher_UnorderedSetBenchmark, benchmark::PriorityLevel::LOW);
BENCHMARK(InputFetcher_SetBenchmark, benchmark::PriorityLevel::LOW);

BENCHMARK(InputFetcher_SortedVectorBenchmark_shortid, benchmark::PriorityLevel::LOW);
BENCHMARK(InputFetcher_UnorderedSetBenchmark_shortid, benchmark::PriorityLevel::LOW);
BENCHMARK(InputFetcher_SetBenchmark_shortid, benchmark::PriorityLevel::LOW);

BENCHMARK(Txid_UnorderedSalted, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_SetOrdered, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorBinarySearch, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorLinearScan, benchmark::PriorityLevel::LOW);

BENCHMARK(Txid_UnorderedSalted_shortid, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_SetOrdered_shortid, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorBinarySearch_shortid, benchmark::PriorityLevel::LOW);
BENCHMARK(Txid_VectorLinearScan_shortid, benchmark::PriorityLevel::LOW);
