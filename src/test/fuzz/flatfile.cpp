// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <flatfile.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

void CheckFlatFilePosContracts(const FlatFilePos& pos)
{
    assert(pos.IsNull() == (pos.nFile == -1));
    const std::string expected{"FlatFilePos(nFile=" + std::to_string(pos.nFile) + ", nPos=" + std::to_string(pos.nPos) + ")"};
    assert(pos.ToString() == expected);

    // Core persists only usable file positions. Null is an in-memory error
    // sentinel and is intentionally excluded from this round-trip contract.
    if (pos.IsNull()) return;

    DataStream stream{};
    stream << pos;
    FlatFilePos roundtrip{};
    stream >> roundtrip;
    assert(roundtrip == pos);
    assert(!roundtrip.IsNull());
}

} // namespace

FUZZ_TARGET(flatfile)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const FlatFilePos null_position{};
    CheckFlatFilePosContracts(null_position);
    const FlatFilePos explicit_null_position{-1, 0};
    assert(explicit_null_position == null_position);

    std::optional<FlatFilePos> flat_file_pos = ConsumeDeserializable<FlatFilePos>(fuzzed_data_provider);
    if (!flat_file_pos) {
        return;
    }
    CheckFlatFilePosContracts(*flat_file_pos);
    std::optional<FlatFilePos> another_flat_file_pos = ConsumeDeserializable<FlatFilePos>(fuzzed_data_provider);
    if (another_flat_file_pos) {
        CheckFlatFilePosContracts(*another_flat_file_pos);
        assert((*flat_file_pos == *another_flat_file_pos) != (*flat_file_pos != *another_flat_file_pos));
    }
}
