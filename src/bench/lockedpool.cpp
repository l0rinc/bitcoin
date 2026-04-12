// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <support/lockedpool.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#define ASIZE 2048
#define MSIZE 2048
static constexpr uint32_t INITIAL_STATE{0x12345678};

static void BenchLockedPool(benchmark::Bench& bench)
{
    void *synth_base = reinterpret_cast<void*>(0x08000000);
    const size_t synth_size = 1024*1024;
    std::optional<Arena> b;

    constexpr size_t kOps = 4096;
    std::vector<void*> addr{ASIZE, nullptr};
    uint32_t s = INITIAL_STATE;
    bench.batch(kOps).epochIterations(1)
        .setup([&] {
            b.emplace(synth_base, synth_size, 16);
            addr.assign(ASIZE, nullptr);
            s = INITIAL_STATE;
        })
        .run([&] {
            assert(s == INITIAL_STATE);
            for (size_t n = 0; n < kOps; ++n) {
                int idx = s & (addr.size() - 1);
                if (s & 0x80000000) {
                    b->free(addr[idx]);
                    addr[idx] = nullptr;
                } else if (!addr[idx]) {
                    addr[idx] = b->alloc((s >> 16) & (MSIZE - 1));
                }
                bool lsb = s & 1;
                s >>= 1;
                if (lsb)
                    s ^= 0xf00f00f0; // LFSR period 0xf7ffffe0
            }
        });
}

BENCHMARK(BenchLockedPool);
