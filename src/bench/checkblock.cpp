// Copyright (c) 2016-present The Bitcoin Core developers
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
#include <util/byte_units.h>
#include <util/chaintype.h>
#include <validation.h>

#include <cassert>
#include <vector>

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    bench.unit("block").run([&] {
        SpanReader stream{benchmark::data::block413567};
        CBlock block;
        stream >> TX_WITH_WITNESS(block);
    });
}

static void DeserializeAndCheckBlockTest(benchmark::Bench& bench)
{
    ArgsManager bench_args;
    const auto chainParams = CreateChainParams(bench_args, ChainType::MAIN);

    bench.unit("block").run([&] {
        SpanReader stream{benchmark::data::block413567};
        CBlock block; // Note that CBlock caches its checked state, so we need to recreate it here
        stream >> TX_WITH_WITNESS(block);

        BlockValidationState validationState;
        bool checked = CheckBlock(block, validationState, chainParams->GetConsensus());
        assert(checked);
    });
}

static void DataStreamInitAndFree(benchmark::Bench& bench)
{
    const std::vector bytes(1_MiB, std::byte{0x42});
    bench.unit("MB").run([&] {
        DataStream v{bytes};
        ankerl::nanobench::doNotOptimizeAway(v.data());
    });
}

BENCHMARK(DeserializeBlockTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(DeserializeAndCheckBlockTest, benchmark::PriorityLevel::HIGH);
BENCHMARK(DataStreamInitAndFree, benchmark::PriorityLevel::HIGH);
