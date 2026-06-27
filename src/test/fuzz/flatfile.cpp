// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <flatfile.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <optional>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

void initialize_flatfile()
{
    static const auto testing_setup{MakeNoLogFileContext<>()};
    g_setup = testing_setup.get();
}

size_t ExpectedAllocation(uint32_t pos, size_t add_size, size_t chunk_size)
{
    const size_t old_chunks{(pos + chunk_size - 1) / chunk_size};
    const size_t new_chunks{(pos + add_size + chunk_size - 1) / chunk_size};
    return new_chunks > old_chunks ? new_chunks * chunk_size - pos : 0;
}

void AssertWriteReadRoundTrip(const FlatFileSeq& seq, const FlatFilePos& pos, const std::vector<uint8_t>& data)
{
    FILE* file{seq.Open(pos)};
    assert(file);
    assert(std::fwrite(data.data(), 1, data.size(), file) == data.size());
    assert(std::fclose(file) == 0);

    file = seq.Open(pos, /*read_only=*/true);
    assert(file);
    std::vector<uint8_t> read_data(data.size());
    assert(std::fread(read_data.data(), 1, read_data.size(), file) == read_data.size());
    assert(std::fclose(file) == 0);
    assert(read_data == data);
}

void AssertReadOnlyMissingDoesNotCreate(const FlatFileSeq& seq, const FlatFilePos& pos)
{
    assert(!fs::exists(seq.FileName(pos)));
    FILE* file{seq.Open(pos, /*read_only=*/true)};
    assert(file == nullptr);
    assert(!fs::exists(seq.FileName(pos)));
}
} // namespace

FUZZ_TARGET(flatfile, .init = initialize_flatfile)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    if (const std::optional<FlatFilePos> flat_file_pos{ConsumeDeserializable<FlatFilePos>(fuzzed_data_provider)}) {
        if (const std::optional<FlatFilePos> another_flat_file_pos{ConsumeDeserializable<FlatFilePos>(fuzzed_data_provider)}) {
            assert((*flat_file_pos == *another_flat_file_pos) != (*flat_file_pos != *another_flat_file_pos));
        }
        assert(flat_file_pos->IsNull() == (flat_file_pos->nFile == -1));
        (void)flat_file_pos->ToString();
    }

    const fs::path data_dir{g_setup->m_args.GetDataDirBase() / "flatfile_fuzz"};
    fs::remove_all(data_dir);
    fs::create_directories(data_dir);

    const size_t chunk_size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 512)};
    FlatFileSeq seq{data_dir, "f", chunk_size};

    assert(seq.Open(FlatFilePos{}, fuzzed_data_provider.ConsumeBool()) == nullptr);
    AssertReadOnlyMissingDoesNotCreate(seq, FlatFilePos{100, 0});

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const FlatFilePos pos{
                    fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, 3),
                    fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096)};
                AssertWriteReadRoundTrip(seq, pos, ConsumeRandomLengthByteVector(fuzzed_data_provider, 256));
            },
            [&] {
                const FlatFilePos pos{
                    fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, 3),
                    fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096)};
                const size_t add_size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 1024)};
                bool out_of_space;
                const size_t allocated{seq.Allocate(pos, add_size, out_of_space)};
                assert(!out_of_space);
                assert(allocated == ExpectedAllocation(pos.nPos, add_size, chunk_size));
                if (allocated > 0) {
                    assert(fs::file_size(seq.FileName(pos)) >= pos.nPos + allocated);
                }
            },
            [&] {
                const FlatFilePos pos{
                    fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, 3),
                    fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096)};
                bool out_of_space;
                (void)seq.Allocate(FlatFilePos{pos.nFile, 0}, std::max<uint32_t>(pos.nPos, 1), out_of_space);
                assert(!out_of_space);
                const bool finalize{fuzzed_data_provider.ConsumeBool()};
                assert(seq.Flush(pos, finalize));
                if (finalize) {
                    assert(fs::file_size(seq.FileName(pos)) == pos.nPos);
                } else {
                    assert(fs::file_size(seq.FileName(pos)) >= pos.nPos);
                }
            },
            [&] {
                const FlatFilePos missing{
                    fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(100, 103),
                    fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096)};
                AssertReadOnlyMissingDoesNotCreate(seq, missing);
            },
            [&] {
                assert(seq.Open(FlatFilePos{}, fuzzed_data_provider.ConsumeBool()) == nullptr);
            });
    }

    fs::remove_all(data_dir);
}
