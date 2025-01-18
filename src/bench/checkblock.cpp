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

static void SizeComputerBlockBench(benchmark::Bench& bench) {
    CBlock block;
    DataStream(benchmark::data::block413567) >> TX_WITH_WITNESS(block);

    bench.unit("block").run([&] {
        SizeComputer size_computer;
        size_computer << TX_WITH_WITNESS(block);
        assert(size_computer.size() == benchmark::data::block413567.size());
    });
}

static void SerializeBlockBench(benchmark::Bench& bench) {
    CBlock block;
    DataStream(benchmark::data::block413567) >> TX_WITH_WITNESS(block);

    // Create output stream and verify first serialization matches input
    bench.unit("block").run([&] {
        DataStream output_stream(benchmark::data::block413567.size());
        output_stream << TX_WITH_WITNESS(block);
        assert(output_stream.size() == benchmark::data::block413567.size());
    });
}

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockBench(benchmark::Bench& bench)
{
    DataStream stream(benchmark::data::block413567);
    std::byte a{0};
    stream.write(std::span{&a, 1}); // Prevent compaction

    bench.unit("block").run([&] {
        CBlock block;
        stream >> TX_WITH_WITNESS(block);
        bool rewound = stream.Rewind(benchmark::data::block413567.size());
        assert(rewound);
    });
}

static void DeserializeAndCheckBlock(benchmark::Bench& bench)
{
    DataStream stream(benchmark::data::block413567);
    std::byte a{0};
    stream.write(std::span{&a, 1}); // Prevent compaction

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

BENCHMARK(SizeComputerBlockBench, benchmark::PriorityLevel::HIGH);
BENCHMARK(SerializeBlockBench, benchmark::PriorityLevel::HIGH);
BENCHMARK(DeserializeBlockBench, benchmark::PriorityLevel::HIGH);
BENCHMARK(DeserializeAndCheckBlock, benchmark::PriorityLevel::HIGH);
