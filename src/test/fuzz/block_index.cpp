// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <node/blockstorage.h>
#include <pow.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <util/byte_units.h>
#include <validation.h>

#include <map>
#include <optional>

using kernel::CBlockFileInfo;

namespace {

const BasicTestingSetup* g_setup;

// Stable merkle root used by generated block index entries.
uint256 g_block_hash;

bool operator==(const CBlockFileInfo& a, const CBlockFileInfo& b)
{
    return a.nBlocks == b.nBlocks &&
        a.nSize == b.nSize &&
        a.nUndoSize == b.nUndoSize &&
        a.nHeightFirst == b.nHeightFirst &&
        a.nHeightLast == b.nHeightLast &&
        a.nTimeFirst == b.nTimeFirst &&
        a.nTimeLast == b.nTimeLast;
}

std::optional<CBlockHeader> ConsumeBlockHeader(FuzzedDataProvider& provider, const uint256& prev_hash, const Consensus::Params& params)
{
    CBlockHeader header;
    header.nVersion = provider.ConsumeIntegral<decltype(header.nVersion)>();
    header.hashPrevBlock = prev_hash;
    header.hashMerkleRoot = g_block_hash;
    header.nTime = provider.ConsumeIntegral<decltype(header.nTime)>();
    header.nBits = Params().GenesisBlock().nBits;
    header.nNonce = provider.ConsumeIntegral<decltype(header.nNonce)>();

    for (int attempt{0}; attempt < 256; ++attempt) {
        if (CheckProofOfWork(header.GetHash(), header.nBits, params)) {
            return header;
        }
        ++header.nNonce;
    }
    return std::nullopt;
}

uint32_t ConsumeBlockStatus(FuzzedDataProvider& provider, const bool have_data)
{
    uint32_t validity{provider.ConsumeIntegralInRange<uint32_t>(BLOCK_VALID_UNKNOWN, BLOCK_VALID_SCRIPTS)};
    if (have_data && validity < BLOCK_VALID_TRANSACTIONS) {
        validity = BLOCK_VALID_TRANSACTIONS;
    }

    uint32_t status{validity};
    if (have_data) {
        status |= BLOCK_HAVE_DATA;
        if (provider.ConsumeBool()) {
            status |= BLOCK_HAVE_UNDO;
        }
        if (provider.ConsumeBool()) {
            status |= BLOCK_OPT_WITNESS;
        }
    }
    if (provider.ConsumeBool()) {
        status |= BLOCK_FAILED_VALID;
    }
    return status;
}

void AssertBlockPositionAccessors(const CBlockIndex& index) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);
    const FlatFilePos block_pos{index.GetBlockPos()};
    if (index.nStatus & BLOCK_HAVE_DATA) {
        assert(block_pos.nFile == index.nFile);
        assert(block_pos.nPos == index.nDataPos);
    } else {
        assert(block_pos.IsNull());
    }

    const FlatFilePos undo_pos{index.GetUndoPos()};
    if (index.nStatus & BLOCK_HAVE_UNDO) {
        assert(index.nStatus & BLOCK_HAVE_DATA);
        assert(undo_pos.nFile == index.nFile);
        assert(undo_pos.nPos == index.nUndoPos);
    } else {
        assert(undo_pos.IsNull());
    }
}

} // namespace

void init_block_index()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::MAIN);
    g_setup = testing_setup.get();
    g_block_hash = Params().GenesisBlock().GetHash();
}

FUZZ_TARGET(block_index, .init = init_block_index)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    auto block_index = kernel::BlockTreeDB(DBParams{
        .path = "", // Memory only.
        .cache_bytes = 1_MiB,
        .memory_only = true,
    });

    const auto params{Params().GetConsensus()};

    // Generate a number of block files to be stored in the index.
    int files_count = fuzzed_data_provider.ConsumeIntegralInRange(1, 100);
    std::vector<std::unique_ptr<CBlockFileInfo>> files;
    files.reserve(files_count);
    std::vector<std::pair<int, const CBlockFileInfo*>> files_info;
    files_info.reserve(files_count);
    for (int i = 0; i < files_count; i++) {
        if (auto file_info = ConsumeDeserializable<CBlockFileInfo>(fuzzed_data_provider)) {
            files.push_back(std::make_unique<CBlockFileInfo>(std::move(*file_info)));
            files_info.emplace_back(i, files.back().get());
        } else {
            return;
        }
    }

    // Generate a number of block headers to be stored in the index.
    int blocks_count = fuzzed_data_provider.ConsumeIntegralInRange(files_count * 10, files_count * 100);
    std::vector<std::unique_ptr<CBlockIndex>> blocks;
    blocks.reserve(blocks_count);
    std::vector<uint256> block_hashes;
    block_hashes.reserve(blocks_count);
    std::vector<const CBlockIndex*> blocks_info;
    blocks_info.reserve(blocks_count);
    for (int i = 0; i < blocks_count; i++) {
        const uint256 prev_hash{i == 0 ? uint256{} : block_hashes.back()};
        auto header{ConsumeBlockHeader(fuzzed_data_provider, prev_hash, params)};
        if (!header) {
            return;
        }
        block_hashes.push_back(header->GetHash());
        blocks.push_back(std::make_unique<CBlockIndex>(*header));
        blocks.back()->phashBlock = &block_hashes.back();
        blocks.back()->pprev = i == 0 ? nullptr : blocks[i - 1].get();
        blocks.back()->nHeight = i;
        const bool have_data{fuzzed_data_provider.ConsumeBool()};
        const uint32_t status{ConsumeBlockStatus(fuzzed_data_provider, have_data)};
        const unsigned int tx_count{(status & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS ?
                                        fuzzed_data_provider.ConsumeIntegral<unsigned int>() | 1U :
                                        0U};
        const int file_num{status & (BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO) ?
                               fuzzed_data_provider.ConsumeIntegralInRange<int>(0, files_count - 1) :
                               0};
        const unsigned int data_pos{status & BLOCK_HAVE_DATA ?
                                        fuzzed_data_provider.ConsumeIntegral<unsigned int>() :
                                        0U};
        const unsigned int undo_pos{status & BLOCK_HAVE_UNDO ?
                                        fuzzed_data_provider.ConsumeIntegral<unsigned int>() :
                                        0U};
        WITH_LOCK(::cs_main, {
            blocks.back()->nStatus = status;
            blocks.back()->nTx = tx_count;
            blocks.back()->nFile = file_num;
            blocks.back()->nDataPos = data_pos;
            blocks.back()->nUndoPos = undo_pos;
            AssertBlockPositionAccessors(*blocks.back());
        });
        blocks_info.push_back(blocks.back().get());
    }

    // Store these files and blocks in the block index. It should not fail.
    block_index.WriteBatchSync(files_info, files_count - 1, blocks_info);

    // We should be able to read every block file info we stored. Its value should correspond to
    // what we stored above.
    CBlockFileInfo info;
    for (const auto& [n, file_info]: files_info) {
        assert(block_index.ReadBlockFileInfo(n, info));
        assert(info == *file_info);
    }

    // We should be able to read the last block file number. Its value should be consistent.
    int last_block_file;
    assert(block_index.ReadLastBlockFile(last_block_file));
    assert(last_block_file == files_count - 1);

    // We should be able to flip and read the reindexing flag.
    bool reindexing;
    block_index.WriteReindexing(true);
    block_index.ReadReindexing(reindexing);
    assert(reindexing);
    block_index.WriteReindexing(false);
    block_index.ReadReindexing(reindexing);
    assert(!reindexing);

    // We should be able to set and read the value of any random flag.
    const std::string flag_name = fuzzed_data_provider.ConsumeRandomLengthString(100);
    for (const bool initial_value : {false, true}) {
        bool flag_value{initial_value};
        assert(!block_index.ReadFlag(flag_name, flag_value));
        assert(flag_value == initial_value);
    }

    block_index.WriteFlag(flag_name, true);
    bool flag_value{false};
    assert(block_index.ReadFlag(flag_name, flag_value));
    assert(flag_value);
    block_index.WriteFlag(flag_name, false);
    flag_value = true;
    assert(block_index.ReadFlag(flag_name, flag_value));
    assert(!flag_value);

    // We should be able to load everything we've previously stored.
    std::map<uint256, CBlockIndex> loaded_blocks;
    const auto inserter = [&](const uint256& hash) {
        if (hash.IsNull()) {
            return static_cast<CBlockIndex*>(nullptr);
        }
        auto [it, inserted]{loaded_blocks.try_emplace(hash)};
        if (inserted) {
            it->second.phashBlock = &it->first;
        }
        return &it->second;
    };
    WITH_LOCK(::cs_main, {
        assert(block_index.LoadBlockIndexGuts(params, inserter, g_setup->m_interrupt));
        assert(loaded_blocks.size() == block_hashes.size());
        for (size_t i{0}; i < blocks.size(); ++i) {
            const CBlockIndex& original{*blocks[i]};
            auto loaded_it{loaded_blocks.find(block_hashes[i])};
            assert(loaded_it != loaded_blocks.end());
            const CBlockIndex& loaded{loaded_it->second};
            assert(loaded.GetBlockHash() == block_hashes[i]);
            assert(loaded.pprev == (i == 0 ? nullptr : &loaded_blocks.at(block_hashes[i - 1])));
            assert(loaded.nHeight == original.nHeight);
            assert(loaded.nFile == original.nFile);
            assert(loaded.nDataPos == original.nDataPos);
            assert(loaded.nUndoPos == original.nUndoPos);
            assert(loaded.nVersion == original.nVersion);
            assert(loaded.hashMerkleRoot == original.hashMerkleRoot);
            assert(loaded.nTime == original.nTime);
            assert(loaded.nBits == original.nBits);
            assert(loaded.nNonce == original.nNonce);
            assert(loaded.nStatus == original.nStatus);
            assert(loaded.nTx == original.nTx);
            assert(loaded.GetBlockHeader().GetHash() == block_hashes[i]);
            AssertBlockPositionAccessors(loaded);
        }
    });
}
