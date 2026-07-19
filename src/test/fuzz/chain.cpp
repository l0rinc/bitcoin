// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace {

void AssertDiskIndexFieldsEqual(const CDiskBlockIndex& actual, const CDiskBlockIndex& expected)
    EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    assert(actual.nHeight == expected.nHeight);
    assert(actual.nStatus == expected.nStatus);
    assert(actual.nTx == expected.nTx);
    assert(actual.nFile == expected.nFile);
    assert(actual.nDataPos == expected.nDataPos);
    assert(actual.nUndoPos == expected.nUndoPos);
    assert(actual.nVersion == expected.nVersion);
    assert(actual.hashPrev == expected.hashPrev);
    assert(actual.hashMerkleRoot == expected.hashMerkleRoot);
    assert(actual.nTime == expected.nTime);
    assert(actual.nBits == expected.nBits);
    assert(actual.nNonce == expected.nNonce);
}

} // namespace

FUZZ_TARGET(chain)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::optional<CDiskBlockIndex> disk_block_index = ConsumeDeserializable<CDiskBlockIndex>(fuzzed_data_provider);
    if (!disk_block_index) {
        return;
    }

    const uint256 zero{};
    disk_block_index->phashBlock = &zero;

    // CDiskBlockIndex is the on-disk boundary. Round-trip every serialized field before
    // exercising the in-memory accessors and validity state machine.
    DataStream serialized;
    serialized << *disk_block_index;
    CDiskBlockIndex round_tripped;
    serialized >> round_tripped;
    assert(serialized.empty());
    {
        LOCK(::cs_main);
        AssertDiskIndexFieldsEqual(round_tripped, *disk_block_index);
    }

    CBlockHeader expected_constructed_header;
    expected_constructed_header.nVersion = disk_block_index->nVersion;
    expected_constructed_header.hashPrevBlock = disk_block_index->hashPrev;
    expected_constructed_header.hashMerkleRoot = disk_block_index->hashMerkleRoot;
    expected_constructed_header.nTime = disk_block_index->nTime;
    expected_constructed_header.nBits = disk_block_index->nBits;
    expected_constructed_header.nNonce = disk_block_index->nNonce;

    {
        LOCK(::cs_main);
        assert(disk_block_index->ConstructBlockHash() == expected_constructed_header.GetHash());
        const FlatFilePos expected_block_pos{disk_block_index->nStatus & BLOCK_HAVE_DATA
                                                 ? FlatFilePos{disk_block_index->nFile, disk_block_index->nDataPos}
                                                 : FlatFilePos{}};
        const FlatFilePos expected_undo_pos{disk_block_index->nStatus & BLOCK_HAVE_UNDO
                                                ? FlatFilePos{disk_block_index->nFile, disk_block_index->nUndoPos}
                                                : FlatFilePos{}};
        assert(disk_block_index->GetBlockPos() == expected_block_pos);
        assert(disk_block_index->GetUndoPos() == expected_undo_pos);
        assert(disk_block_index->GetBlockTime() == static_cast<int64_t>(disk_block_index->nTime));
        assert(disk_block_index->GetBlockTimeMax() == static_cast<int64_t>(disk_block_index->nTimeMax));
        assert(disk_block_index->GetMedianTimePast() == disk_block_index->GetBlockTime());
        assert(disk_block_index->HaveNumChainTxs() == (disk_block_index->m_chain_tx_count != 0));
        const bool expected_valid{!(disk_block_index->nStatus & BLOCK_FAILED_VALID) &&
                                  (disk_block_index->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS};
        assert(disk_block_index->IsValid(BLOCK_VALID_TRANSACTIONS) == expected_valid);
    }

    const CBlockHeader block_header = disk_block_index->GetBlockHeader();
    assert(block_header.nVersion == disk_block_index->nVersion);
    assert(block_header.hashPrevBlock.IsNull());
    assert(block_header.hashMerkleRoot == disk_block_index->hashMerkleRoot);
    assert(block_header.nTime == disk_block_index->nTime);
    assert(block_header.nBits == disk_block_index->nBits);
    assert(block_header.nNonce == disk_block_index->nNonce);

    {
        LOCK(::cs_main);
        const CDiskBlockIndex copied{*disk_block_index};
        AssertDiskIndexFieldsEqual(copied, *disk_block_index);
    }
    (void)disk_block_index->BuildSkip();

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const BlockStatus block_status = fuzzed_data_provider.PickValueInArray({
            BlockStatus::BLOCK_VALID_UNKNOWN,
            BlockStatus::BLOCK_VALID_RESERVED,
            BlockStatus::BLOCK_VALID_TREE,
            BlockStatus::BLOCK_VALID_TRANSACTIONS,
            BlockStatus::BLOCK_VALID_CHAIN,
            BlockStatus::BLOCK_VALID_SCRIPTS,
            BlockStatus::BLOCK_VALID_MASK,
            BlockStatus::BLOCK_HAVE_DATA,
            BlockStatus::BLOCK_HAVE_UNDO,
            BlockStatus::BLOCK_HAVE_MASK,
            BlockStatus::BLOCK_FAILED_VALID,
            BlockStatus::BLOCK_OPT_WITNESS,
        });
        if (block_status & ~BLOCK_VALID_MASK) {
            continue;
        }
        const uint32_t status_before{WITH_LOCK(::cs_main, return disk_block_index->nStatus)};
        const bool expected_changed{!(status_before & BLOCK_FAILED_VALID) &&
                                    (status_before & BLOCK_VALID_MASK) < block_status};
        const bool changed{WITH_LOCK(::cs_main, return disk_block_index->RaiseValidity(block_status))};
        const uint32_t status_after{WITH_LOCK(::cs_main, return disk_block_index->nStatus)};
        assert(changed == expected_changed);
        if (status_before & BLOCK_FAILED_VALID) {
            assert(status_after == status_before);
        } else {
            assert((status_after & ~BLOCK_VALID_MASK) == (status_before & ~BLOCK_VALID_MASK));
            assert((status_after & BLOCK_VALID_MASK) == std::max(status_before & BLOCK_VALID_MASK, static_cast<uint32_t>(block_status)));
        }
    }

    CBlockIndex block_index{block_header};
    block_index.phashBlock = &zero;
    assert(block_index.GetBlockHash() == zero);
    const CBlockHeader reconstructed_header{block_index.GetBlockHeader()};
    assert(reconstructed_header.nVersion == block_header.nVersion);
    assert(reconstructed_header.hashPrevBlock == block_header.hashPrevBlock);
    assert(reconstructed_header.hashMerkleRoot == block_header.hashMerkleRoot);
    assert(reconstructed_header.nTime == block_header.nTime);
    assert(reconstructed_header.nBits == block_header.nBits);
    assert(reconstructed_header.nNonce == block_header.nNonce);
    (void)block_index.ToString();
}
