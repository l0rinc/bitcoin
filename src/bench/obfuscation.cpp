// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <random.h>
#include <span.h>
#include <streams.h>
#include <util/byte_units.h>

#include <cstddef>
#include <vector>

static void ObfuscationBench(benchmark::Bench& bench)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    constexpr size_t bytes{1024};
    auto data{rng.randbytes<std::byte>(bytes)};

    const Obfuscation obfuscation{rng.rand64()};

    size_t offset{0};
    bench.batch(data.size()).unit("byte").run([&] {
        obfuscation(data, offset++); // mutated differently each time
        ankerl::nanobench::doNotOptimizeAway(data);
    });
}

BENCHMARK(ObfuscationBench, benchmark::PriorityLevel::HIGH);
