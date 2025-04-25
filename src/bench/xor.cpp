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
    constexpr size_t bytes{10_MiB};
    auto test_data{rng.randbytes<std::byte>(bytes)};

    std::array<std::byte, sizeof(uint64_t)> obfuscation{};
    rng.fillrand(obfuscation);

    size_t offset{0};
    bench.batch(bytes / 1_MiB).unit("MiB").run([&] {
        util::Obfuscate(test_data, obfuscation, offset++);
        ankerl::nanobench::doNotOptimizeAway(test_data);
    });
}

BENCHMARK(ObfuscationBench, benchmark::PriorityLevel::HIGH);
