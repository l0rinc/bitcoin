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

FUZZ_TARGET(parse_hd_keypath)
{
    const std::string keypath_str(buffer.begin(), buffer.end());
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::vector<uint32_t> original_keypath = ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider);

    std::vector<uint32_t> keypath{original_keypath};
    const bool parsed{ParseHDKeypath(keypath_str, keypath)};
    if (!parsed) {
        assert(keypath == original_keypath);
    } else {
        const std::string rewritten{WriteHDKeypath(keypath)};
        std::vector<uint32_t> reparsed;
        assert(ParseHDKeypath(rewritten, reparsed));
        assert(reparsed == keypath);
    }

    const std::vector<uint32_t> random_keypath = ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider);
    const std::string formatted_h{FormatHDKeypath(random_keypath)};
    assert(random_keypath.empty() == formatted_h.empty());
    assert(formatted_h.empty() || formatted_h.front() == '/');
    assert(WriteHDKeypath(random_keypath) == "m" + formatted_h);

    for (const bool apostrophe : {false, true}) {
        const std::string written{WriteHDKeypath(random_keypath, apostrophe)};
        std::vector<uint32_t> parsed_keypath{ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider)};
        assert(ParseHDKeypath(written, parsed_keypath));
        assert(parsed_keypath == random_keypath);
    }
}
