// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/fs.h>
#include <util/obfuscation.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

void initialize_buffered_file()
{
    static const auto testing_setup{MakeNoLogFileContext<>()};
    g_setup = testing_setup.get();
}

std::optional<size_t> FindNextByte(std::span<const std::byte> bytes, size_t pos, size_t limit, std::byte target)
{
    const size_t end{std::min(bytes.size(), limit)};
    if (pos >= end) return std::nullopt;
    const auto begin{bytes.begin() + pos};
    const auto found{std::find(begin, bytes.begin() + end, target)};
    if (found == bytes.begin() + end) return std::nullopt;
    return std::distance(bytes.begin(), found);
}

void AssertBufferedFileModel(FuzzedDataProvider& fuzzed_data_provider)
{
    const auto key_bytes{ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Obfuscation::KEY_SIZE)};
    const Obfuscation obfuscation{std::span{key_bytes}.first<Obfuscation::KEY_SIZE>()};
    const std::vector<std::byte> file_bytes{ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider, 4096)};

    const fs::path data_dir{g_setup->m_args.GetDataDirBase() / "buffered_file_fuzz"};
    fs::remove_all(data_dir);
    fs::create_directories(data_dir);
    const fs::path path{data_dir / "input"};

    {
        AutoFile write_file{fsbridge::fopen(path, "wb"), obfuscation};
        assert(!write_file.IsNull());
        write_file.write(file_bytes);
        assert(write_file.fclose() == 0);
    }

    AutoFile read_file{fsbridge::fopen(path, "rb"), obfuscation};
    assert(!read_file.IsNull());

    const uint64_t buffer_size{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(1, 512)};
    const uint64_t rewind_size{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, buffer_size - 1)};
    BufferedFile buffered_file{read_file, buffer_size, rewind_size};
    uint64_t read_limit{std::numeric_limits<uint64_t>::max()};

    const auto effective_end = [&] {
        return std::min<uint64_t>(file_bytes.size(), read_limit);
    };

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const size_t read_size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 512)};
                const uint64_t before{buffered_file.GetPos()};
                std::vector<std::byte> actual(read_size);
                try {
                    buffered_file.read(actual);
                    assert(before + read_size <= effective_end());
                    assert(buffered_file.GetPos() == before + read_size);
                    assert(std::equal(actual.begin(), actual.end(), file_bytes.begin() + before));
                } catch (const std::ios_base::failure&) {
                    assert(before + read_size > effective_end());
                    assert(buffered_file.GetPos() <= effective_end());
                }
            },
            [&] {
                const uint64_t before{buffered_file.GetPos()};
                const uint64_t skip{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 512)};
                try {
                    buffered_file.SkipTo(before + skip);
                    assert(before + skip <= effective_end());
                    assert(buffered_file.GetPos() == before + skip);
                } catch (const std::ios_base::failure&) {
                    assert(before + skip > effective_end());
                    assert(buffered_file.GetPos() <= effective_end());
                }
            },
            [&] {
                const uint64_t limit{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, file_bytes.size() + 512)};
                const uint64_t before{buffered_file.GetPos()};
                const uint64_t old_limit{read_limit};
                const bool success{buffered_file.SetLimit(limit)};
                assert(success == (limit >= before));
                assert(buffered_file.GetPos() == before);
                read_limit = success ? limit : old_limit;
                assert(buffered_file.GetPos() <= read_limit);
            },
            [&] {
                buffered_file.SetLimit();
                read_limit = std::numeric_limits<uint64_t>::max();
                assert(buffered_file.GetPos() <= read_limit);
            },
            [&] {
                const uint64_t requested{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, file_bytes.size() + 512)};
                const bool success{buffered_file.SetPos(requested)};
                assert(success == (buffered_file.GetPos() == requested));
                assert(buffered_file.GetPos() <= effective_end());
            },
            [&] {
                const uint64_t before{buffered_file.GetPos()};
                const std::byte target{fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
                const std::optional<size_t> expected{FindNextByte(file_bytes, before, read_limit, target)};
                try {
                    buffered_file.FindByte(target);
                    assert(expected);
                    assert(buffered_file.GetPos() == *expected);
                    std::byte found{};
                    buffered_file.read({&found, 1});
                    assert(found == target);
                    assert(buffered_file.SetPos(*expected));
                } catch (const std::ios_base::failure&) {
                    assert(!expected);
                    assert(buffered_file.GetPos() <= effective_end());
                }
            });
    }

    assert(read_file.fclose() == 0);
    fs::remove_all(data_dir);
}
} // namespace

FUZZ_TARGET(buffered_file, .init = initialize_buffered_file)
{
    FuzzedDataProvider model_provider{buffer.data(), buffer.size()};
    AssertBufferedFileModel(model_provider);

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    std::optional<BufferedFile> opt_buffered_file;
    const auto key_bytes{ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Obfuscation::KEY_SIZE)};
    AutoFile fuzzed_file{
        fuzzed_file_provider.open(),
        Obfuscation{std::span{key_bytes}.first<Obfuscation::KEY_SIZE>()},
    };
    try {
        auto n_buf_size = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096);
        auto n_rewind_in = fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096);
        opt_buffered_file.emplace(fuzzed_file, n_buf_size, n_rewind_in);
    } catch (const std::ios_base::failure&) {
    }
    if (opt_buffered_file && !fuzzed_file.IsNull()) {
        std::optional<uint64_t> read_limit;
        const auto assert_position_within_limit = [&] {
            if (read_limit) {
                assert(opt_buffered_file->GetPos() <= *read_limit);
            }
        };
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 100)
        {
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    std::array<std::byte, 4096> arr{};
                    try {
                        opt_buffered_file->read({arr.data(), fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)});
                    } catch (const std::ios_base::failure&) {
                    }
                    assert_position_within_limit();
                },
                [&] {
                    const uint64_t limit{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096)};
                    if (opt_buffered_file->SetLimit(limit)) {
                        read_limit = limit;
                    }
                    assert_position_within_limit();
                },
                [&] {
                    opt_buffered_file->SetLimit();
                    read_limit = std::nullopt;
                },
                [&] {
                    const uint64_t pos{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 4096)};
                    const bool success{opt_buffered_file->SetPos(pos)};
                    assert(success == (opt_buffered_file->GetPos() == pos));
                    assert_position_within_limit();
                },
                [&] {
                    const std::byte target{fuzzed_data_provider.ConsumeIntegral<uint8_t>()};
                    try {
                        opt_buffered_file->FindByte(target);
                        const uint64_t found_pos{opt_buffered_file->GetPos()};
                        std::byte found{};
                        opt_buffered_file->read({&found, 1});
                        assert(found == target);
                        assert(opt_buffered_file->SetPos(found_pos));
                    } catch (const std::ios_base::failure&) {
                    }
                    assert_position_within_limit();
                },
                [&] {
                    ReadFromStream(fuzzed_data_provider, *opt_buffered_file);
                    assert_position_within_limit();
                });
        }
        opt_buffered_file->GetPos();
    }
}
