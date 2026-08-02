// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <node/utxo_snapshot.h>
#include <span.h>
#include <streams.h>
#include <sync.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <util/fs.h>

#include <cstdint>
#include <vector>

void initialize_utxo_snapshot_base_blockhash()
{
    static const auto setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET(utxo_snapshot_base_blockhash, .init = initialize_utxo_snapshot_base_blockhash)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> data{fuzzed_data_provider.ConsumeBytes<uint8_t>(4096)};

    const fs::path chainstate_dir = gArgs.GetDataDirNet() / "fuzzed_snapshot_chainstate";
    fs::remove_all(chainstate_dir);
    Assert(fs::create_directories(chainstate_dir));

    {
        const fs::path blockhash_file = chainstate_dir / node::SNAPSHOT_BLOCKHASH_FILENAME;
        AutoFile file{fsbridge::fopen(blockhash_file, "wb")};
        Assert(!file.IsNull());
        file << std::span{data};
        Assert(file.fclose() == 0);
    }

    LOCK(cs_main);
    const auto base_blockhash = node::ReadSnapshotBaseBlockhash(chainstate_dir);
    Assert(base_blockhash.has_value() == (data.size() >= uint256::size()));
    fs::remove_all(chainstate_dir);
}
