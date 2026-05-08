// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <support/lockedpool.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#define ASIZE 2048
#define MSIZE 2048

static void BenchLockedPool(benchmark::Bench& bench)
{
    const size_t synth_size = 1024*1024;
    std::vector<std::byte> memory(synth_size);
    Arena b(memory.data(), synth_size, 16);

    std::vector<void*> addr{ASIZE, nullptr};
    uint32_t s = 0x12345678;
    bench.run([&] {
        int idx = s & (addr.size() - 1);
        if (s & 0x80000000) {
            b.free(addr[idx]);
            addr[idx] = nullptr;
        } else if (!addr[idx]) {
            addr[idx] = b.alloc((s >> 16) & (MSIZE - 1));
        }
        bool lsb = s & 1;
        s >>= 1;
        if (lsb)
            s ^= 0xf00f00f0; // LFSR period 0xf7ffffe0
    });
    for (void *ptr: addr)
        b.free(ptr);
    addr.clear();
}

BENCHMARK(BenchLockedPool);
