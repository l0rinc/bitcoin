// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <logging.h>
#include <threadpool.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <atomic>
#include <cstdint>
#include <vector>

static void init_pool_fuzz() { LogInstance().DisableLogging(); }

FUZZ_TARGET(threadpool, .init = init_pool_fuzz)
{
    FuzzedDataProvider fdp(buffer.data(), buffer.size());

    ThreadPool pool{fdp.ConsumeIntegralInRange<size_t>(1, 100)};
    std::atomic<uint32_t> ok{0};
    for (size_t r = 0, rounds = fdp.ConsumeIntegralInRange<size_t>(0, 256); r < rounds; ++r) {
        pool.Run([&](size_t i){
            if (fdp.ConsumeBool()) throw std::runtime_error("fuzz-throw");
            ok.fetch_add(i, std::memory_order_relaxed);
        });
        // If an exception escaped, we would hang at the second barrier, so reaching here is success.
    }
    (void)ok.load(std::memory_order_relaxed);
}
