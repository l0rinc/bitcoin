// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <clientversion.h>
#include <flatfile.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/fs.h>
#include <util/time.h>
#include <validation.h>

#include <cassert>
#include <cstdio>
#include <cstdint>
#include <vector>

namespace {
const TestingSetup* g_setup;

void WriteBlockFile(const fs::path& path, const DataStream& stream)
{
    FILE* outfile{fsbridge::fopen(path, "wb")};
    assert(outfile);
    assert(std::fwrite(stream.data(), 1, stream.size(), outfile) == stream.size());
    assert(std::fclose(outfile) == 0);
}

void AssertUnknownParentImport(FuzzedDataProvider& fuzzed_data_provider)
{
    ChainstateManager& chainman{*Assert(g_setup->m_node.chainman)};
    const CChainParams& params{chainman.GetParams()};
    const MessageStartChars& message_start{params.MessageStart()};

    std::vector<std::byte> prefix{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 64)};
    for (auto& byte : prefix) {
        if (byte == std::byte{message_start[0]}) {
            byte = std::byte{static_cast<uint8_t>(message_start[0] ^ 0xff)};
        }
    }

    CBlockHeader header;
    header.nVersion = 4;
    header.hashPrevBlock = uint256::ONE;
    header.hashMerkleRoot = ConsumeUInt256(fuzzed_data_provider);
    header.nTime = params.GenesisBlock().nTime + 1 + fuzzed_data_provider.ConsumeIntegral<uint16_t>();
    header.nBits = params.GenesisBlock().nBits;
    header.nNonce = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    assert(header.hashPrevBlock != params.GenesisBlock().GetHash());
    assert(header.GetHash() != params.GenesisBlock().GetHash());

    DataStream stream{prefix};
    stream << message_start;
    stream << static_cast<uint32_t>(GetSerializeSize(header));
    stream << header;

    const fs::path blkfile{g_setup->m_args.GetDataDirBase() / "load_external_block_file_unknown_parent.dat"};
    fs::remove(blkfile);
    WriteBlockFile(blkfile, stream);

    AutoFile file{fsbridge::fopen(blkfile, "rb")};
    assert(!file.IsNull());

    const CBlockIndex* tip_before{WITH_LOCK(::cs_main, return chainman.ActiveTip())};
    assert(tip_before);
    const int file_num{fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
    FlatFilePos pos{file_num, 0};
    std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
    chainman.LoadExternalBlockFile(file, &pos, &blocks_with_unknown_parent);

    assert(WITH_LOCK(::cs_main, return chainman.ActiveTip()) == tip_before);
    assert(blocks_with_unknown_parent.size() == 1);
    const auto& [parent_hash, child_pos] = *blocks_with_unknown_parent.begin();
    assert(parent_hash == header.hashPrevBlock);
    assert(child_pos.nFile == file_num);
    assert(child_pos.nPos == prefix.size() + node::STORAGE_HEADER_BYTES);
    assert(pos.nFile == file_num);
    assert(pos.nPos == child_pos.nPos);
    {
        LOCK(::cs_main);
        assert(!chainman.m_blockman.LookupBlockIndex(header.hashPrevBlock));
        assert(!chainman.m_blockman.LookupBlockIndex(header.GetHash()));
    }

    fs::remove(blkfile);
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
    AssertUnknownParentImport(fuzzed_data_provider);
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile fuzzed_block_file{fuzzed_file_provider.open()};
    if (fuzzed_block_file.IsNull()) {
        return;
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        // Corresponds to the -reindex case (track orphan blocks across files).
        FlatFilePos flat_file_pos{0, 0};
        std::multimap<uint256, FlatFilePos> blocks_with_unknown_parent;
        g_setup->m_node.chainman->LoadExternalBlockFile(fuzzed_block_file, &flat_file_pos, &blocks_with_unknown_parent);
        for (const auto& [_, child_pos] : blocks_with_unknown_parent) {
            assert(!child_pos.IsNull());
            assert(child_pos.nFile == 0);
            assert(child_pos.nPos >= 8);
        }
    } else {
        // Corresponds to the -loadblock= case (orphan blocks aren't tracked across files).
        g_setup->m_node.chainman->LoadExternalBlockFile(fuzzed_block_file);
    }
}
