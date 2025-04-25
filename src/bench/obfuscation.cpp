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

    constexpr size_t key_size{sizeof(uint64_t)};
    std::array<std::byte, key_size> obfuscation{};
    rng.fillrand(obfuscation);

    size_t offset{0};
    bench.batch(data.size()).unit("byte").run([&] {
        util::Obfuscate(data, obfuscation, offset++); // mutated differently each time
        ankerl::nanobench::doNotOptimizeAway(data);
    });
}

BENCHMARK(ObfuscationBench, benchmark::PriorityLevel::HIGH);
