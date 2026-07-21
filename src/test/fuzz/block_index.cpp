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
#include <memory>

using kernel::CBlockFileInfo;

namespace {

const BasicTestingSetup* g_setup;

// Hardcoded block hash and nBits to make sure the blocks we store pass the pow check.
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

struct BlockIndexState final {
    uint256 hash;
    uint256 hash_prev;
    int nHeight;
    int nFile;
    unsigned int nDataPos;
    unsigned int nUndoPos;
    int32_t nVersion;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
    uint32_t nStatus;
    unsigned int nTx;

    friend bool operator==(const BlockIndexState&, const BlockIndexState&) = default;
};

BlockIndexState CaptureBlockIndex(const CBlockIndex& block)
{
    LOCK(::cs_main);
    return {
        block.GetBlockHash(),
        block.pprev ? block.pprev->GetBlockHash() : uint256{},
        block.nHeight,
        block.nFile,
        block.nDataPos,
        block.nUndoPos,
        block.nVersion,
        block.hashMerkleRoot,
        block.nTime,
        block.nBits,
        block.nNonce,
        block.nStatus,
        block.nTx,
    };
}

void AssertBlockIndexState(const CBlockIndex& actual, const BlockIndexState& expected)
{
    Assert(CaptureBlockIndex(actual) == expected);
}

std::optional<CBlockHeader> ConsumeBlockHeader(FuzzedDataProvider& provider)
{
    CBlockHeader header;
    header.nVersion = provider.ConsumeIntegral<decltype(header.nVersion)>();
    header.hashPrevBlock = g_block_hash;
    header.hashMerkleRoot = g_block_hash;
    header.nTime = provider.ConsumeIntegral<decltype(header.nTime)>();
    header.nBits = Params().GenesisBlock().nBits;
    header.nNonce = provider.ConsumeIntegral<decltype(header.nNonce)>();
    // Regtest's easy target lets the fuzzer retain arbitrary header fields
    // while ensuring every stored record passes LoadBlockIndexGuts' PoW check.
    for (int attempts{0}; attempts < 1000; ++attempts) {
        if (CheckProofOfWork(header.GetHash(), header.nBits, Params().GetConsensus())) {
            return header;
        }
        ++header.nNonce;
    }
    return std::nullopt;
}

} // namespace

void init_block_index()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::REGTEST);
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
    std::vector<const CBlockIndex*> blocks_info;
    blocks_info.reserve(blocks_count);
    CBlockIndex genesis;
    genesis.phashBlock = &g_block_hash;
    std::map<uint256, uint256> block_hashes;
    std::map<uint256, BlockIndexState> expected_blocks;
    expected_blocks.emplace(g_block_hash, CaptureBlockIndex(genesis));
    for (int i = 0; i < blocks_count; i++) {
        const auto header{ConsumeBlockHeader(fuzzed_data_provider)};
        if (!header) return;
        blocks.push_back(std::make_unique<CBlockIndex>(*header));
        blocks.back()->pprev = &genesis;
        block_hashes.emplace(blocks.back()->GetBlockHeader().GetHash(), uint256{});
        const auto hash_it = block_hashes.find(blocks.back()->GetBlockHeader().GetHash());
        Assert(hash_it != block_hashes.end());
        blocks.back()->phashBlock = &hash_it->first;
        blocks_info.push_back(blocks.back().get());
        expected_blocks[blocks.back()->GetBlockHash()] = CaptureBlockIndex(*blocks.back());
    }

    // Store these files and blocks in the block index. It should not fail.
    block_index.WriteBatchSync(files_info, files_count - 1, blocks_info);

    // We should be able to read every block file info we stored. Its value should correspond to
    // what we stored above.
    CBlockFileInfo info;
    for (const auto& [n, file_info]: files_info) {
        Assert(block_index.ReadBlockFileInfo(n, info));
        Assert(info == *file_info);
    }

    // We should be able to read the last block file number. Its value should be consistent.
    int last_block_file;
    Assert(block_index.ReadLastBlockFile(last_block_file));
    Assert(last_block_file == files_count - 1);

    // We should be able to flip and read the reindexing flag.
    bool reindexing;
    block_index.WriteReindexing(true);
    block_index.ReadReindexing(reindexing);
    Assert(reindexing);
    block_index.WriteReindexing(false);
    block_index.ReadReindexing(reindexing);
    Assert(!reindexing);

    // We should be able to set and read the value of any random flag.
    const std::string flag_name = fuzzed_data_provider.ConsumeRandomLengthString(100);
    bool flag_value;
    block_index.WriteFlag(flag_name, true);
    block_index.ReadFlag(flag_name, flag_value);
    Assert(flag_value);
    block_index.WriteFlag(flag_name, false);
    block_index.ReadFlag(flag_name, flag_value);
    Assert(!flag_value);

    // We should be able to load everything we've previously stored. Note to assert on the
    // return value we need to make sure all blocks pass the pow check.
    const auto params{Params().GetConsensus()};
    std::map<uint256, std::unique_ptr<CBlockIndex>> loaded_blocks;
    const auto inserter = [&](const uint256& hash) {
        auto [it, inserted] = loaded_blocks.try_emplace(hash, std::make_unique<CBlockIndex>());
        if (inserted) it->second->phashBlock = &it->first;
        return it->second.get();
    };
    Assert(WITH_LOCK(::cs_main, return block_index.LoadBlockIndexGuts(params, inserter, g_setup->m_interrupt)));
    Assert(loaded_blocks.size() == expected_blocks.size());
    for (const auto& [hash, expected] : expected_blocks) {
        const auto it = loaded_blocks.find(hash);
        Assert(it != loaded_blocks.end());
        AssertBlockIndexState(*it->second, expected);
    }
}
