// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <clientversion.h>
#include <flatfile.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/time.h>
#include <validation.h>

#include <cstdint>
#include <vector>

namespace {
const TestingSetup* g_setup;

struct BlockIndexIdentity {
    CBlockIndex* index;
    CBlockIndex* pprev;
    int nHeight;
    arith_uint256 nChainWork;
    int32_t nVersion;
    uint256 hash;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;
};

std::vector<BlockIndexIdentity> CaptureBlockIndexIdentities(ChainstateManager& chainman)
{
    LOCK(cs_main);
    std::vector<BlockIndexIdentity> identities;
    identities.reserve(chainman.BlockIndex().size());
    for (auto& [hash, index] : chainman.BlockIndex()) {
        identities.push_back({
            .index = &index,
            .pprev = index.pprev,
            .nHeight = index.nHeight,
            .nChainWork = index.nChainWork,
            .nVersion = index.nVersion,
            .hash = hash,
            .hashMerkleRoot = index.hashMerkleRoot,
            .nTime = index.nTime,
            .nBits = index.nBits,
            .nNonce = index.nNonce,
        });
    }
    return identities;
}

void AssertBlockIndexImportContracts(ChainstateManager& chainman, const std::vector<BlockIndexIdentity>& before)
{
    LOCK(cs_main);
    const auto& block_index = chainman.BlockIndex();

    // Import may add entries and update storage/status fields, but it must not rewrite the
    // identity or ancestry of an entry that was already in the block tree.
    for (const auto& expected : before) {
        const auto it = block_index.find(expected.hash);
        assert(it != block_index.end());
        const CBlockIndex& actual = it->second;
        assert(&actual == expected.index);
        assert(actual.GetBlockHash() == expected.hash);
        assert(actual.pprev == expected.pprev);
        assert(actual.nHeight == expected.nHeight);
        assert(actual.nChainWork == expected.nChainWork);
        assert(actual.nVersion == expected.nVersion);
        assert(actual.hashMerkleRoot == expected.hashMerkleRoot);
        assert(actual.nTime == expected.nTime);
        assert(actual.nBits == expected.nBits);
        assert(actual.nNonce == expected.nNonce);
    }

    // Every indexed entry must retain a parent pointer that resolves to the same tree. The active
    // tip may change when an imported file contains a better chain, but it must remain indexed.
    for (const auto& [hash, index] : block_index) {
        assert(index.GetBlockHash() == hash);
        if (index.pprev) {
            const auto parent = block_index.find(index.pprev->GetBlockHash());
            assert(parent != block_index.end());
            assert(&parent->second == index.pprev);
            assert(index.nHeight == index.pprev->nHeight + 1);
            assert(index.nChainWork >= index.pprev->nChainWork);
        }
    }
    if (const CBlockIndex* active_tip = chainman.ActiveTip()) {
        const auto it = block_index.find(active_tip->GetBlockHash());
        assert(it != block_index.end());
        assert(&it->second == active_tip);
    }
}
} // namespace

void initialize_load_external_block_file()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(load_external_block_file, .init = initialize_load_external_block_file)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile fuzzed_block_file{fuzzed_file_provider.open()};
    if (fuzzed_block_file.IsNull()) {
        return;
    }
    ChainstateManager& chainman{*g_setup->m_node.chainman};
    const auto block_index_before{CaptureBlockIndexIdentities(chainman)};
    if (fuzzed_data_provider.ConsumeBool()) {
        // Corresponds to the -reindex case (track orphan blocks across files).
        FlatFilePos flat_file_pos;
        std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
        chainman.LoadExternalBlockFile(fuzzed_block_file, &flat_file_pos, &blocks_with_unknown_parent);
    } else {
        // Corresponds to the -loadblock= case (orphan blocks aren't tracked across files).
        chainman.LoadExternalBlockFile(fuzzed_block_file);
    }
    AssertBlockIndexImportContracts(chainman, block_index_before);
}
