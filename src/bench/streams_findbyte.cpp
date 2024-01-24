// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>

static void FindByte(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const BasicTestingSetup>(ChainType::REGTEST)};
    const fs::path path{testing_setup->m_path_root / "streams_tmp"};
    AutoFile file{fsbridge::fopen(path, "w+b")};
    const size_t file_size = 200;
    uint8_t data[file_size] = {0};
    data[file_size - 1] = 1;
    file << data;
    file.seek(0, SEEK_SET);
    BufferedFile bf{file, /*nBufSize=*/file_size + 1, /*nRewindIn=*/file_size};

    bench.setup([&] { bf.SetPos(0); })
        .run([&] { bf.FindByte(std::byte(1)); });

    // Cleanup
    assert(file.fclose() == 0);
    fs::remove(path);
}

BENCHMARK(FindByte);
