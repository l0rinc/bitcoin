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
    for (const bool apostrophe : {false, true}) {
        const std::string formatted{FormatHDKeypath(random_keypath, apostrophe)};
        assert(random_keypath.empty() == formatted.empty());
        assert(formatted.empty() || formatted.front() == '/');
        assert(formatted.empty() || formatted.back() != '/');
        assert(formatted.find("//") == std::string::npos);
        assert(WriteHDKeypath(random_keypath, apostrophe) == "m" + formatted);

        std::vector<uint32_t> parsed_formatted{ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider)};
        assert(ParseHDKeypath(formatted.empty() ? formatted : formatted.substr(1), parsed_formatted));
        assert(parsed_formatted == random_keypath);

        const std::string written{WriteHDKeypath(random_keypath, apostrophe)};
        assert(written == "m" || (written.size() > 1 && written[0] == 'm' && written[1] == '/'));
        assert(written.find("//") == std::string::npos);
        std::vector<uint32_t> parsed_keypath{ConsumeRandomLengthIntegralVector<uint32_t>(fuzzed_data_provider)};
        assert(ParseHDKeypath(written, parsed_keypath));
        assert(parsed_keypath == random_keypath);
    }
}
