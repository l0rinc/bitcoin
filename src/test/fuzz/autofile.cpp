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
#include <cstdio>
#include <iostream>
#include <optional>
#include <vector>

namespace {
std::optional<int64_t> TryTell(AutoFile& auto_file)
{
    try {
        return auto_file.tell();
    } catch (const std::ios_base::failure&) {
        return std::nullopt;
    }
}

void AssertTellFails(AutoFile& auto_file)
{
    assert(auto_file.IsNull());
    bool threw{false};
    try {
        (void)auto_file.tell();
    } catch (const std::ios_base::failure&) {
        threw = true;
    }
    assert(threw);
}
} // namespace

FUZZ_TARGET(autofile)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    const auto key_bytes{ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Obfuscation::KEY_SIZE)};
    AutoFile auto_file{
        fuzzed_file_provider.open(),
        Obfuscation{std::span{key_bytes}.first<Obfuscation::KEY_SIZE>()},
    };
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 100) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                std::array<std::byte, 4096> arr{};
                try {
                    const size_t read_size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)};
                    const std::optional<int64_t> before{TryTell(auto_file)};
                    auto_file.read({arr.data(), read_size});
                    if (before) assert(TryTell(auto_file) == *before + read_size);
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                const std::array<std::byte, 4096> arr{};
                try {
                    const size_t write_size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)};
                    const std::optional<int64_t> before{TryTell(auto_file)};
                    auto_file.write({arr.data(), write_size});
                    if (before) assert(TryTell(auto_file) == *before + write_size);
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                try {
                    const size_t ignore_size{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096)};
                    const std::optional<int64_t> before{TryTell(auto_file)};
                    auto_file.ignore(ignore_size);
                    if (before) assert(TryTell(auto_file) == *before + ignore_size);
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                (void)auto_file.fclose();
                AssertTellFails(auto_file);
            },
            [&] {
                try {
                    const int64_t pos{fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, 4096)};
                    auto_file.seek(pos, SEEK_SET);
                    assert(TryTell(auto_file) == pos);
                } catch (const std::ios_base::failure&) {
                }
            },
            [&] {
                try {
                    const std::optional<int64_t> before{TryTell(auto_file)};
                    const int64_t size{auto_file.size()};
                    assert(size >= 0);
                    if (before) assert(TryTell(auto_file) == *before);
                } catch (const std::ios_base::failure&) {
                    if (auto_file.IsNull()) AssertTellFails(auto_file);
                }
            },
            [&] {
                const std::optional<int64_t> pos{TryTell(auto_file)};
                if (auto_file.IsNull()) {
                    assert(!pos);
                } else if (pos) {
                    assert(*pos >= 0);
                }
            },
            [&] {
                ReadFromStream(fuzzed_data_provider, auto_file);
            },
            [&] {
                WriteToStream(fuzzed_data_provider, auto_file);
            });
    }
    (void)auto_file.IsNull();
    if (fuzzed_data_provider.ConsumeBool()) {
        FILE* f = auto_file.release();
        AssertTellFails(auto_file);
        if (f != nullptr) {
            fclose(f);
        }
    } else {
        (void)auto_file.fclose();
        AssertTellFails(auto_file);
    }
}
