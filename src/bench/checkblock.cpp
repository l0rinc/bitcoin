// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <validation.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <utility>
#include <vector>

using SizeWeight = std::pair<size_t, size_t>;

static std::vector<unsigned char> ByteVector(size_t size, unsigned char tag)
{
    return std::vector(size, tag);
}

static CScript ScriptOfSize(size_t size, unsigned char tag)
{
    const auto bytes{ByteVector(size, tag)};
    return {bytes.begin(), bytes.end()};
}

template <size_t N>
static size_t WeightedSize(const std::array<SizeWeight, N>& weights, size_t index)
{
    size_t total{0};
    for (const auto& [_, weight] : weights) {
        total += weight;
    }
    index %= total;
    for (const auto& [size, weight] : weights) {
        if (index < weight) return size;
        index -= weight;
    }
    assert(false);
    return 0;
}

static CTransactionRef MakeSizedTransaction(size_t index)
{
    CMutableTransaction tx;
    const auto tag{static_cast<unsigned char>(index)};
    constexpr std::array prevector_sizes{
        SizeWeight{0, 20},
        SizeWeight{23, 19},
        SizeWeight{25, 18},
        SizeWeight{22, 14},
        SizeWeight{106, 8},
        SizeWeight{107, 7},
        SizeWeight{34, 6},
        SizeWeight{35, 2},
        SizeWeight{33, 2},
        SizeWeight{71, 2},
        SizeWeight{72, 1},
        SizeWeight{64, 1},
    };
    constexpr std::array vector_sizes{
        SizeWeight{33, 37},
        SizeWeight{71, 29},
        SizeWeight{72, 15},
        SizeWeight{64, 7},
        SizeWeight{0, 5},
        SizeWeight{105, 3},
        SizeWeight{65, 2},
        SizeWeight{37, 2},
    };

    tx.vin.emplace_back(COutPoint{}, ScriptOfSize(WeightedSize(prevector_sizes, index * 3), tag));
    tx.vout.emplace_back(0, ScriptOfSize(WeightedSize(prevector_sizes, index * 3 + 1), tag));
    tx.vout.emplace_back(0, ScriptOfSize(WeightedSize(prevector_sizes, index * 3 + 2), tag));
    tx.vin[0].scriptWitness.stack.push_back(ByteVector(WeightedSize(vector_sizes, index * 2), tag));
    if (index % 4 != 0) {
        tx.vin[0].scriptWitness.stack.push_back(ByteVector(WeightedSize(vector_sizes, index * 2 + 1), tag));
    }
    return MakeTransactionRef(std::move(tx));
}

// The real block fixture below predates witness data. Use a mixed block for
// serialization-only benchmarks so the hot byte-vector sizes seen in modern
// block data are exercised.
static CBlock CreateSerializationBenchmarkBlock()
{
    CBlock block;
    block.vtx.reserve(4096);
    while (block.vtx.size() < 4096) {
        block.vtx.push_back(MakeSizedTransaction(block.vtx.size()));
    }
    return block;
}

static void SizeComputerBlock(benchmark::Bench& bench)
{
    const CBlock block{CreateSerializationBenchmarkBlock()};
    DataStream expected_stream;
    expected_stream << TX_WITH_WITNESS(block);
    const auto expected_size{expected_stream.size()};

    bench.unit("block").run([&] {
        SizeComputer size_computer;
        size_computer << TX_WITH_WITNESS(block);
        assert(size_computer.size() == expected_size);
    });
}

static void SerializeBlock(benchmark::Bench& bench)
{
    const CBlock block{CreateSerializationBenchmarkBlock()};
    DataStream expected_stream;
    expected_stream << TX_WITH_WITNESS(block);
    const auto expected_size{expected_stream.size()};

    bench.unit("block").run([&] {
        DataStream output_stream;
        output_stream.reserve(expected_size);
        output_stream << TX_WITH_WITNESS(block);
        assert(output_stream.size() == expected_size);
    });
}

// These are the two major time-sinks which happen after we have fully received
// a block off the wire, but before we can relay the block on to peers using
// compact block relay.

static void DeserializeBlockTest(benchmark::Bench& bench)
{
    const auto block_data{benchmark::data::block413567};
    bench.unit("block").run([&] {
        CBlock block;
        SpanReader{block_data} >> TX_WITH_WITNESS(block);
        assert(block.vtx.size() == 1557);
    });
}

static void CheckBlockTest(benchmark::Bench& bench)
{
    const auto& chain_params{CChainParams::Main()};
    const auto block_data{benchmark::data::block413567};

    CBlock block;
    bench.unit("block")
        .setup([&] {
            block = CBlock{};
            SpanReader{block_data} >> TX_WITH_WITNESS(block);
            assert(block.vtx.size() == 1557);
        })
        .run([&] {
            BlockValidationState validationState;
            const bool checked{CheckBlock(block, validationState, chain_params->GetConsensus())};
            assert(checked);
        });
}

BENCHMARK(SizeComputerBlock);
BENCHMARK(SerializeBlock);
BENCHMARK(DeserializeBlockTest);
BENCHMARK(CheckBlockTest);
