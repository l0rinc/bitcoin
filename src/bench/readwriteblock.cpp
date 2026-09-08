// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <flatfile.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <validation.h>

#include <memory>
#include <optional>
#include <span>

static CBlock CreateTestBlock()
{
    CBlock block;
    SpanReader{benchmark::data::block413567} >> TX_WITH_WITNESS(block);
    return block;
}

static void WriteBlockBench(benchmark::Bench& bench)
{
    std::unique_ptr<const TestingSetup> testing_setup;
    const CBlock block{CreateTestBlock()};
    bench.setup([&] {
             // WriteBlock advances the block-file cursor and preallocates storage. Recreate the
             // setup outside the timed region so each measured write starts from the same state.
             testing_setup.reset();
             testing_setup = MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN);
         })
        .run([&] {
            auto& blockman{testing_setup->m_node.chainman->m_blockman};
            LOCK(::cs_main);
            const auto pos{blockman.WriteBlock(block, 413'567)};
            assert(!pos.IsNull());
        });
}

static void ReadBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto& test_block{CreateTestBlock()};
    const auto& expected_hash{test_block.GetHash()};
    const auto& pos{WITH_LOCK(::cs_main, return blockman.WriteBlock(test_block, 413'567))};
    bench.run([&] {
        CBlock block;
        const auto success{blockman.ReadBlock(block, pos, expected_hash)};
        assert(success);
    });
}

static void ReadRawBlockBench(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const TestingSetup>(ChainType::MAIN)};
    auto& blockman{testing_setup->m_node.chainman->m_blockman};
    const auto pos{WITH_LOCK(::cs_main, return blockman.WriteBlock(CreateTestBlock(), 413'567))};
    bench.run([&] {
        const auto res{blockman.ReadRawBlock(pos)};
        assert(res);
    });
}

BENCHMARK(WriteBlockBench);
BENCHMARK(ReadBlockBench);
BENCHMARK(ReadRawBlockBench);
