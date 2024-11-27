// Copyright (c) 2016-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <span.h>
#include <streams.h>
#include <util/chaintype.h>
#include <validation.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

static void SerializeBlockTest(benchmark::Bench& bench) {
    CBlock block;
    DataStream(benchmark::data::block413567) >> TX_WITH_WITNESS(block);

    // Create output stream and verify first serialization matches input
    bench.unit("block").run([&] {
        DataStream output_stream;
        output_stream << TX_WITH_WITNESS(block);
        assert(output_stream.size() == benchmark::data::block413567.size());
    });
}

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    DataStream stream(benchmark::data::block413567);
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    bench.unit("block").run([&] {
        CBlock block;
        stream >> TX_WITH_WITNESS(block);
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
        assert(rewound);
    });
}

static void DeserializeAndCheckBlockTest(benchmark::Bench& bench)
{
    DataStream stream(benchmark::data::block413567);
    std::byte a{0};
    stream.write({&a, 1}); // Prevent compaction

    ArgsManager bench_args;
    const auto chainParams = CreateChainParams(bench_args, ChainType::MAIN);

    bench.unit("block").run([&] {
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> TX_WITH_WITNESS(block);
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
        assert(rewound);

        BlockValidationState validationState;
        bool checked = CheckBlock(block, validationState, chainParams->GetConsensus());
        assert(checked);
    });
}

BENCHMARK(SerializeBlockTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(DeserializeBlockTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(DeserializeAndCheckBlockTest, benchmark::PriorityLevel::HIGH);

/*
Before:
|            ns/block |             block/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|        1,350,185.16 |              740.64 |    0.2% |     10.99 | `DeserializeAndCheckBlockTest`
|        1,031,343.47 |              969.61 |    0.2% |     11.01 | `DeserializeBlockTest`
|          355,079.94 |            2,816.27 |    0.9% |     10.90 | `SerializeBlockTest`

After:
|            ns/block |             block/s |    err% |     total | benchmark
|--------------------:|--------------------:|--------:|----------:|:----------
|        1,312,111.25 |              762.13 |    0.7% |     10.87 | `DeserializeAndCheckBlockTest`
|          997,010.74 |            1,003.00 |    0.5% |     10.94 | `DeserializeBlockTest`
|          328,847.66 |            3,040.92 |    1.7% |     10.80 | `SerializeBlockTest`
*/