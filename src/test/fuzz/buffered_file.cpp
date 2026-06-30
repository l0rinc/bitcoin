// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/util.h>
#include <util/obfuscation.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

FUZZ_TARGET(buffered_file)
{
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
                    try {
                        opt_buffered_file->FindByte(std::byte(fuzzed_data_provider.ConsumeIntegral<uint8_t>()));
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
