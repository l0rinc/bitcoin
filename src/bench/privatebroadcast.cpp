// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit/.

#include <bench/bench.h>
#include <bench/data/block413567.raw.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <private_broadcast.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>

#include <cassert>
#include <vector>

using namespace std::ranges;

static CBlock CreateTestBlock()
{
    CBlock block;
    DataStream{benchmark::data::block413567} >> TX_WITH_WITNESS(block);
    return block;
}

static std::vector<CTransactionRef> GetBlockTxs()
{
    const CBlock block{CreateTestBlock()};
    std::vector<CTransactionRef> txs;
    txs.reserve(block.vtx.size());
    for (size_t i{1}; i < block.vtx.size(); ++i) {
        txs.push_back(block.vtx[i]);
    }
    shuffle(txs, FastRandomContext{/*fDeterministic=*/true});
    txs.resize(MAX_PRIVATE_BROADCAST_CONNECTIONS);
    return txs;
}

static void PrivateBroadcastBench(benchmark::Bench& bench)
{
    const std::vector additions{GetBlockTxs()};

    std::vector removals{additions};
    shuffle(removals, FastRandomContext{/*fDeterministic=*/true});

    PrivateBroadcast pb;
    bench.batch(additions.size()).run([&] {
        assert(!pb.GetTxForBroadcast());
        for (const auto& tx : additions) {
            assert(pb.Add(tx));
        }

        for (size_t i{0}; i < additions.size(); ++i) {
            const NodeId nodeid(i);
            pb.PushedToNode(nodeid, additions[i]->GetHash());
            assert(pb.GetTxPushedToNode(NodeId(i)));
        }

        for (size_t i{0}; i < additions.size(); ++i) {
            assert(pb.FinishBroadcast(i, /*confirmed_by_node=*/i % 10));
        }

        assert(pb.GetStale().size() <= additions.size());

        for (const auto& tx : removals) {
            assert(pb.GetTxForBroadcast());
            assert(pb.Remove(tx));
        }
        assert(!pb.GetTxForBroadcast());
    });
}

BENCHMARK(PrivateBroadcastBench, benchmark::PriorityLevel::HIGH);
