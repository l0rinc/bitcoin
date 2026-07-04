// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <blockencodings.h>
#include <consensus/merkle.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/mempool.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/check.h>
#include <util/time.h>
#include <util/translation.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
const TestingSetup* g_setup;
} // namespace

class FuzzedCBlockHeaderAndShortTxIDs : public CBlockHeaderAndShortTxIDs
{
public:
    using CBlockHeaderAndShortTxIDs::CBlockHeaderAndShortTxIDs;

    void AddPrefilledTx(uint16_t index, CTransactionRef tx)
    {
        prefilledtxn.push_back({index, std::move(tx)});
    }

    void ResizePrefilledTx(size_t size)
    {
        prefilledtxn.resize(size);
    }

    size_t ShortTxIDCount() const { return shorttxids.size(); }

    void ReplaceShortTxID(size_t index, uint64_t shortid)
    {
        assert(index < shorttxids.size());
        shorttxids[index] = shortid;
    }

    std::vector<std::optional<CTransactionRef>> PrefilledTxsByPosition() const
    {
        std::vector<std::optional<CTransactionRef>> ret(BlockTxCount());
        int32_t last_prefilled_index{-1};
        for (const auto& prefilled : prefilledtxn) {
            last_prefilled_index += prefilled.index + 1;
            if (last_prefilled_index < 0 || static_cast<size_t>(last_prefilled_index) >= ret.size()) break;
            ret[last_prefilled_index] = prefilled.tx;
        }
        return ret;
    }

    std::vector<std::optional<uint64_t>> ShortTxIDsByPosition() const
    {
        std::vector<std::optional<uint64_t>> ret(BlockTxCount());
        const auto prefilled_by_position{PrefilledTxsByPosition()};
        size_t short_txid_pos{0};
        for (size_t pos{0}; pos < ret.size(); ++pos) {
            if (prefilled_by_position[pos]) continue;
            assert(short_txid_pos < shorttxids.size());
            ret[pos] = shorttxids[short_txid_pos++];
        }
        assert(short_txid_pos == shorttxids.size());
        return ret;
    }
};

void initialize_pdb()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

PartiallyDownloadedBlock::IsBlockMutatedFn FuzzedIsBlockMutated(bool result)
{
    return [result](const CBlock& block, bool) {
        return result;
    };
}

class FuzzedPartiallyDownloadedBlock : public PartiallyDownloadedBlock
{
public:
    using PartiallyDownloadedBlock::PartiallyDownloadedBlock;

    size_t AvailableTxCount() const
    {
        size_t count{0};
        for (const auto& tx : txn_available) {
            if (tx) ++count;
        }
        return count;
    }

    size_t PrefilledCount() const { return prefilled_count; }
    size_t MempoolCount() const { return mempool_count; }
    size_t ExtraCount() const { return extra_count; }
    const CTransactionRef& AvailableTx(size_t index) const
    {
        assert(index < txn_available.size());
        return txn_available[index];
    }
};

FUZZ_TARGET(partially_downloaded_block, .init = initialize_pdb)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    auto block{ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS)};
    if (!block || block->vtx.size() == 0 ||
        block->vtx.size() >= std::numeric_limits<uint16_t>::max()) {
        return;
    }
    for (const auto& tx : block->vtx) {
        Assert(tx != nullptr);
    }

    FuzzedCBlockHeaderAndShortTxIDs cmpctblock{*block, fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
    const bool force_invalid_init{fuzzed_data_provider.ConsumeBool()};
    if (force_invalid_init) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CMutableTransaction null_tx;
                cmpctblock.AddPrefilledTx(0, MakeTransactionRef(std::move(null_tx)));
            },
            [&] {
                cmpctblock.ResizePrefilledTx(2);
            },
            [&] {
                cmpctblock.AddPrefilledTx(std::numeric_limits<uint16_t>::max(), block->vtx.front());
            },
            [&] {
                cmpctblock.AddPrefilledTx(static_cast<uint16_t>(block->vtx.size()), block->vtx.front());
            });
    }

    bilingual_str error;
    CTxMemPool pool{MemPoolOptionsForTest(g_setup->m_node), error};
    Assert(error.empty());
    FuzzedPartiallyDownloadedBlock pdb{&pool};

    std::vector<std::pair<Wtxid, CTransactionRef>> extra_txn;
    for (size_t i = 1; i < block->vtx.size(); ++i) {
        auto tx{block->vtx[i]};

        bool add_to_extra_txn{fuzzed_data_provider.ConsumeBool()};
        bool add_to_mempool{fuzzed_data_provider.ConsumeBool()};

        if (add_to_extra_txn) {
            extra_txn.emplace_back(tx->GetWitnessHash(), tx);
        }

        if (add_to_mempool && !pool.exists(tx->GetHash())) {
            LOCK2(cs_main, pool.cs);
            TryAddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, *tx));
        }
    }

    if (cmpctblock.ShortTxIDCount() > 0 && fuzzed_data_provider.ConsumeBool()) {
        const Wtxid empty_wtxid{Wtxid::FromUint256(uint256::ZERO)};
        extra_txn.emplace_back(empty_wtxid, CTransactionRef{});
        cmpctblock.ReplaceShortTxID(
            fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, cmpctblock.ShortTxIDCount() - 1),
            cmpctblock.GetShortID(empty_wtxid));
    }

    auto init_status{pdb.InitData(cmpctblock, extra_txn)};
    if (init_status != READ_STATUS_OK) {
        assert(pdb.header.IsNull());
        assert(pdb.AvailableTxCount() == 0);
        assert(pdb.PrefilledCount() == 0);
        assert(pdb.MempoolCount() == 0);
        assert(pdb.ExtraCount() == 0);

        CBlock rejected_block;
        assert(pdb.FillBlock(rejected_block, {}, fuzzed_data_provider.ConsumeBool()) == READ_STATUS_INVALID);
        return;
    }
    assert(pdb.AvailableTxCount() == pdb.PrefilledCount() + pdb.MempoolCount());
    assert(pdb.ExtraCount() <= pdb.MempoolCount());

    const auto prefilled_by_position{cmpctblock.PrefilledTxsByPosition()};
    const auto shorttxids_by_position{cmpctblock.ShortTxIDsByPosition()};

    std::vector<CTransactionRef> missing;
    // Whether we skipped a transaction that should be included in `missing`.
    // FillBlock should never return READ_STATUS_OK if that is the case.
    bool skipped_missing{false};
    size_t required_missing_count{0};
    for (size_t i = 0; i < cmpctblock.BlockTxCount(); i++) {
        const bool tx_available{pdb.IsTxAvailable(i)};
        const CTransactionRef& available_tx{pdb.AvailableTx(i)};
        assert(static_cast<bool>(available_tx) == tx_available);
        if (tx_available) {
            if (prefilled_by_position[i]) {
                assert(available_tx == *prefilled_by_position[i]);
            } else {
                assert(shorttxids_by_position[i]);
                assert(cmpctblock.GetShortID(available_tx->GetWitnessHash()) == *shorttxids_by_position[i]);
            }
        }

        bool skip{fuzzed_data_provider.ConsumeBool()};
        if (!tx_available) {
            ++required_missing_count;
            if (!skip) {
                missing.push_back(block->vtx[i]);
            }
        }

        skipped_missing |= (!tx_available && skip);
    }

    const bool extra_missing{!skipped_missing && fuzzed_data_provider.ConsumeBool()};
    if (extra_missing) {
        missing.push_back(block->vtx[fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, block->vtx.size() - 1)]);
    }

    bool segwit_active{fuzzed_data_provider.ConsumeBool()};

    // Mock IsBlockMutated
    bool fail_block_mutated{fuzzed_data_provider.ConsumeBool()};
    pdb.m_check_block_mutated_mock = FuzzedIsBlockMutated(fail_block_mutated);

    CBlock reconstructed_block{*block};
    reconstructed_block.vtx = {block->vtx[0]};
    const uint256 reconstructed_block_hash{reconstructed_block.GetHash()};
    const std::vector<CTransactionRef> reconstructed_block_txs{reconstructed_block.vtx};
    assert(std::all_of(missing.cbegin(), missing.cend(),
        [](const auto& tx) { return tx != nullptr; }));
    auto fill_status{pdb.FillBlock(reconstructed_block, missing, segwit_active)};
    switch (fill_status) {
    case READ_STATUS_OK:
        assert(!skipped_missing);
        assert(!extra_missing);
        assert(missing.size() == required_missing_count);
        assert(!fail_block_mutated);
        assert(block->GetHash() == reconstructed_block.GetHash());
        assert(block->vtx.size() == reconstructed_block.vtx.size());
        for (size_t i{0}; i < block->vtx.size(); ++i) {
            assert(block->vtx[i]->GetHash() == reconstructed_block.vtx[i]->GetHash());
            assert(block->vtx[i]->GetWitnessHash() == reconstructed_block.vtx[i]->GetWitnessHash());
        }
        break;
    case READ_STATUS_FAILED:
        assert(fail_block_mutated);
        assert(reconstructed_block.GetHash() == reconstructed_block_hash);
        assert(reconstructed_block.vtx == reconstructed_block_txs);
        break;
    case READ_STATUS_INVALID:
        assert(reconstructed_block.GetHash() == reconstructed_block_hash);
        assert(reconstructed_block.vtx == reconstructed_block_txs);
        break;
    }

    assert(pdb.header.IsNull());
    assert(pdb.AvailableTxCount() == 0);
    assert(pdb.PrefilledCount() == 0);
    assert(pdb.MempoolCount() == 0);
    assert(pdb.ExtraCount() == 0);
    CBlock retry_block;
    assert(pdb.FillBlock(retry_block, {}, segwit_active) == READ_STATUS_INVALID);
}
