// src/bench/swiftsync.cpp
// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see COPYING.

#include <bench/bench.h>
#include <swiftsync.h>
#include <util/fs.h>

#include <cstddef>

static void SwiftSyncLoadBench(benchmark::Bench& bench)
{
    // TODO generate a fake swiftsync file
    // SwiftSyncHints hints;
    // hints.Load(fs::PathToString("/Users/lorinc/Dev/swiftsync-888888_new.bin"));
    // assert(hints.IsLoaded() && hints.GetTerminalBlockHeight() == 888'888);
    //
    // bench.batch(hints.GetTerminalBlockHeight()).unit("blocks").run([&] {
    //     size_t utxos{0};
    //     for (int h{0}; h <= hints.GetTerminalBlockHeight(); ++h) {
    //         hints.SetCurrentBlockHeight(h);
    //         while (hints.HasNextBit()) {
    //             utxos += !hints.GetNextBit();
    //         }
    //     }
    //     assert(utxos == 3'071'308'286);
    // });
}

BENCHMARK(SwiftSyncLoadBench, benchmark::PriorityLevel::HIGH);
