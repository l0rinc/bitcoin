// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <random.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <map>
#include <set>
#include <span>
#include <vector>

namespace {

constexpr uint64_t MEMPOOL_DUMP_VERSION_NO_XOR_KEY{1};
constexpr size_t MEMPOOL_TX_COUNT{1'000};
constexpr size_t MEMPOOL_DELTA_COUNT{200};

struct MempoolPersistData {
    std::vector<CTransactionRef> txs;
    std::map<Txid, CAmount> map_deltas;
    std::set<Txid> unbroadcast_txids;
    std::vector<std::byte> serialized;
};

template <typename Stream>
void SerializeMempoolPersistData(Stream& stream, const MempoolPersistData& data)
{
    stream << MEMPOOL_DUMP_VERSION_NO_XOR_KEY;
    stream << static_cast<uint64_t>(data.txs.size());
    for (const auto& tx : data.txs) {
        stream << TX_WITH_WITNESS(*tx);
        stream << int64_t{0};
        stream << int64_t{0};
    }
    stream << data.map_deltas;
    stream << data.unbroadcast_txids;
}

template <typename Stream>
void UnserializeMempoolPersistData(Stream& stream, size_t expected_tx_count, size_t expected_delta_count, size_t expected_unbroadcast_count)
{
    uint64_t version;
    stream >> version;
    assert(version == MEMPOOL_DUMP_VERSION_NO_XOR_KEY);

    uint64_t tx_count;
    stream >> tx_count;
    assert(tx_count == expected_tx_count);

    for (uint64_t i{0}; i < tx_count; ++i) {
        CTransactionRef tx;
        int64_t time;
        int64_t fee_delta;
        stream >> TX_WITH_WITNESS(tx);
        stream >> time;
        stream >> fee_delta;
        assert(tx != nullptr);
    }

    std::map<Txid, CAmount> map_deltas;
    stream >> map_deltas;
    assert(map_deltas.size() == expected_delta_count);

    std::set<Txid> unbroadcast_txids;
    stream >> unbroadcast_txids;
    assert(unbroadcast_txids.size() == expected_unbroadcast_count);
}

MempoolPersistData CreateMempoolPersistData()
{
    FastRandomContext det_rand{true};
    auto testing_setup{MakeNoLogFileContext<TestChain100Setup>(ChainType::REGTEST)};
    CTxMemPool& pool{*testing_setup->m_node.mempool};

    MempoolPersistData data;
    data.txs = testing_setup->PopulateMempool(det_rand, MEMPOOL_TX_COUNT, /*submit=*/true);
    assert(data.txs.size() == MEMPOOL_TX_COUNT);

    data.unbroadcast_txids.clear();
    for (size_t i{0}; i < data.txs.size(); i += 2) {
        data.unbroadcast_txids.insert(data.txs[i]->GetHash());
    }

    for (size_t i{0}; i < MEMPOOL_DELTA_COUNT; ++i) {
        data.map_deltas.emplace(Txid::FromUint256(det_rand.rand256()), static_cast<CAmount>(i + 1));
    }
    assert(data.map_deltas.size() == MEMPOOL_DELTA_COUNT);

    DataStream stream;
    SerializeMempoolPersistData(stream, data);
    data.serialized.assign(stream.begin(), stream.end());

    assert(pool.size() == MEMPOOL_TX_COUNT);
    return data;
}

} // namespace

static void MempoolPersistSerialize(benchmark::Bench& bench)
{
    const auto data{CreateMempoolPersistData()};

    bench.batch(data.txs.size()).unit("tx").run([&] {
        DataStream stream;
        stream.reserve(data.serialized.size());
        SerializeMempoolPersistData(stream, data);
        assert(stream.size() == data.serialized.size());
    });
}

static void MempoolPersistDeserialize(benchmark::Bench& bench)
{
    const auto data{CreateMempoolPersistData()};

    bench.batch(data.txs.size()).unit("tx").run([&] {
        SpanReader stream{std::span<const std::byte>{data.serialized.data(), data.serialized.size()}};
        UnserializeMempoolPersistData(stream, data.txs.size(), data.map_deltas.size(), data.unbroadcast_txids.size());
        assert(stream.empty());
    });
}

BENCHMARK(MempoolPersistSerialize);
BENCHMARK(MempoolPersistDeserialize);
