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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
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

    // Set of available transactions (mempool or extra_txn)
    std::set<uint16_t> available;
    // The coinbase is always available
    available.insert(0);

    std::vector<std::pair<Wtxid, CTransactionRef>> extra_txn;
    for (size_t i = 1; i < block->vtx.size(); ++i) {
        auto tx{block->vtx[i]};

        bool add_to_extra_txn{fuzzed_data_provider.ConsumeBool()};
        bool add_to_mempool{fuzzed_data_provider.ConsumeBool()};

        if (add_to_extra_txn) {
            extra_txn.emplace_back(tx->GetWitnessHash(), tx);
            available.insert(i);
        }

        if (add_to_mempool && !pool.exists(tx->GetHash())) {
            LOCK2(cs_main, pool.cs);
            TryAddToMempool(pool, ConsumeTxMemPoolEntry(fuzzed_data_provider, *tx));
            available.insert(i);
        }
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

    std::vector<CTransactionRef> missing;
    // Whether we skipped a transaction that should be included in `missing`.
    // FillBlock should never return READ_STATUS_OK if that is the case.
    bool skipped_missing{false};
    size_t required_missing_count{0};
    for (size_t i = 0; i < cmpctblock.BlockTxCount(); i++) {
        const bool tx_available{pdb.IsTxAvailable(i)};
        // If init_status == READ_STATUS_OK then a available transaction in the
        // compact block (i.e. IsTxAvailable(i) == true) implies that we marked
        // that transaction as available above (i.e. available.contains(i)).
        // The reverse is not true, due to possible compact block short id
        // collisions (i.e. available.contains(i) does not imply
        // IsTxAvailable(i) == true).
        if (init_status == READ_STATUS_OK) {
            assert(!tx_available || available.contains(i));
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

    CBlock reconstructed_block;
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
        break;
    case READ_STATUS_INVALID:
        break;
    }

    if (missing.size() >= required_missing_count) {
        assert(pdb.header.IsNull());
        assert(pdb.AvailableTxCount() == 0);
        assert(pdb.PrefilledCount() == 0);
        assert(pdb.MempoolCount() == 0);
        assert(pdb.ExtraCount() == 0);
        CBlock retry_block;
        assert(pdb.FillBlock(retry_block, {}, segwit_active) == READ_STATUS_INVALID);
    } else {
        assert(fill_status == READ_STATUS_INVALID);
        assert(!pdb.header.IsNull());
    }
}
