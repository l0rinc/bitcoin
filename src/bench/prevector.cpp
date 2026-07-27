// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <prevector.h>

#include <bench/bench.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>

#include <span>
#include <type_traits>
#include <vector>

struct nontrivial_t
{
    int x{-1};
    nontrivial_t() = default;
    SERIALIZE_METHODS(nontrivial_t, obj) { READWRITE(obj.x); }
};

static_assert(!std::is_trivially_default_constructible_v<nontrivial_t>,
              "expected nontrivial_t to not be trivially constructible");

typedef unsigned char trivial_t;
static_assert(std::is_trivially_default_constructible_v<trivial_t>,
              "expected trivial_t to be trivially constructible");

template <typename T>
static void PrevectorDestructor(benchmark::Bench& bench)
{
    bench.batch(2).run([&] {
        prevector<CScriptBase::STATIC_SIZE, T> t0;
        prevector<CScriptBase::STATIC_SIZE, T> t1;
        t0.resize(CScriptBase::STATIC_SIZE);
        t1.resize(CScriptBase::STATIC_SIZE + 1);
        // Keep the resize stores from being optimized away: for trivial T the
        // compiler can otherwise eliminate them as dead stores, making this
        // benchmark measure only harness overhead (plus malloc/free for t1).
        ankerl::nanobench::doNotOptimizeAway(t0);
        ankerl::nanobench::doNotOptimizeAway(t1);
    });
}

template <typename T>
static void PrevectorClear(benchmark::Bench& bench)
{
    prevector<CScriptBase::STATIC_SIZE, T> t0;
    prevector<CScriptBase::STATIC_SIZE, T> t1;
    bench.batch(2).run([&] {
        t0.resize(CScriptBase::STATIC_SIZE);
        t1.resize(CScriptBase::STATIC_SIZE + 1);
        // Barriers must fire while the vectors are full: after clear() the
        // fill stores are semantically dead and the compiler eliminates them,
        // making this benchmark measure only harness overhead (doubling the
        // work left ns/op unchanged before this fix).
        ankerl::nanobench::doNotOptimizeAway(t0);
        ankerl::nanobench::doNotOptimizeAway(t1);
        t0.clear();
        t1.clear();
    });
}

template <typename T>
static void PrevectorResize(benchmark::Bench& bench)
{
    prevector<CScriptBase::STATIC_SIZE, T> t0;
    prevector<CScriptBase::STATIC_SIZE, T> t1;
    bench.batch(4).run([&] {
        t0.resize(CScriptBase::STATIC_SIZE);
        t1.resize(CScriptBase::STATIC_SIZE + 1);
        // Barriers must fire while the vectors are full: after resize(0) the
        // fill stores are semantically dead and the compiler eliminates them,
        // making this benchmark measure only harness overhead (doubling the
        // resizes left ns/op unchanged at 3.13 before this fix).
        ankerl::nanobench::doNotOptimizeAway(t0);
        ankerl::nanobench::doNotOptimizeAway(t1);
        t0.resize(0);
        t1.resize(0);
    });
}

template <typename T>
static void PrevectorDeserialize(benchmark::Bench& bench)
{
    DataStream data{};
    prevector<CScriptBase::STATIC_SIZE, T> t0;
    t0.resize(CScriptBase::STATIC_SIZE);
    for (auto x = 0; x < 900; ++x) {
        data << t0;
    }
    t0.resize(100);
    for (auto x = 0; x < 100; ++x) {
        data << t0;
    }
    bench.batch(1000).run([&] {
        SpanReader s0{data};
        prevector<CScriptBase::STATIC_SIZE, T> t1;
        for (auto x = 0; x < 1000; ++x) {
            s0 >> t1;
        }
    });
}

template <typename T>
static void PrevectorFillVectorDirect(benchmark::Bench& bench)
{
    bench.run([&] {
        std::vector<prevector<CScriptBase::STATIC_SIZE, T>> vec;
        vec.reserve(260);
        for (size_t i = 0; i < 260; ++i) {
            vec.emplace_back();
        }
    });
}


template <typename T>
static void PrevectorFillVectorIndirect(benchmark::Bench& bench)
{
    bench.run([&] {
        std::vector<prevector<CScriptBase::STATIC_SIZE, T>> vec;
        vec.reserve(260);
        for (size_t i = 0; i < 260; ++i) {
            // force allocation
            vec.emplace_back(CScriptBase::STATIC_SIZE + 1, T{});
        }
    });
}

#define PREVECTOR_TEST(name)                                         \
    static void Prevector##name##Nontrivial(benchmark::Bench& bench) \
    {                                                                \
        Prevector##name<nontrivial_t>(bench);                        \
    }                                                                \
    BENCHMARK(Prevector##name##Nontrivial);                          \
    static void Prevector##name##Trivial(benchmark::Bench& bench)    \
    {                                                                \
        Prevector##name<trivial_t>(bench);                           \
    }                                                                \
    BENCHMARK(Prevector##name##Trivial);

PREVECTOR_TEST(Clear)
PREVECTOR_TEST(Destructor)
PREVECTOR_TEST(Resize)
PREVECTOR_TEST(Deserialize)
PREVECTOR_TEST(FillVectorDirect)
PREVECTOR_TEST(FillVectorIndirect)
