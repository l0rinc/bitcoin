// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/bip32.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
void AssertHDKeypathRoundTrips(const std::vector<uint32_t>& keypath)
{
    std::vector<uint32_t> roundtrip;
    const auto assert_roundtrip = [&](const std::string& path_string) {
        assert(ParseHDKeypath(path_string, roundtrip));
        assert(roundtrip == keypath);
        roundtrip.clear();
    };

    assert_roundtrip(FormatHDKeypath(keypath));
    assert_roundtrip(FormatHDKeypath(keypath, /*apostrophe=*/true));
    assert_roundtrip(WriteHDKeypath(keypath));
    assert_roundtrip(WriteHDKeypath(keypath, /*apostrophe=*/true));
}
} // namespace

FUZZ_TARGET(parse_hd_keypath)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string keypath_str = fuzzed_data_provider.ConsumeRandomLengthString();
    std::vector<uint32_t> keypath;
    if (ParseHDKeypath(keypath_str, keypath)) {
        AssertHDKeypathRoundTrips(keypath);
    }

    const std::vector<uint32_t> random_keypath = ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider);
    AssertHDKeypathRoundTrips(random_keypath);
}
