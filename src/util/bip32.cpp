// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bip32.h>

#include <tinyformat.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <cstdint>
#include <cstdio>
#include <optional>
#include <sstream>
#include <utility>

namespace {
constexpr uint32_t BIP32_HARDENED_KEY_LIMIT{uint32_t{1} << 31};
constexpr uint32_t BIP32_NON_HARDENED_KEY_MASK{BIP32_HARDENED_KEY_LIMIT - 1};
} // namespace

bool ParseHDKeypath(const std::string& keypath_str, std::vector<uint32_t>& keypath)
{
    std::vector<uint32_t> parsed_keypath;
    std::stringstream ss(keypath_str);
    std::string item;
    bool first = true;
    while (std::getline(ss, item, '/')) {
        if (item.compare("m") == 0) {
            if (first) {
                first = false;
                continue;
            }
            return false;
        }
        // Finds whether it is hardened
        uint32_t path = 0;
        size_t pos = item.find_first_of("'h");
        if (pos != std::string::npos) {
            // The hardening marker can only be in the last index of the string
            if (pos != item.size() - 1) {
                return false;
            }
            path |= BIP32_HARDENED_KEY_LIMIT;
            item = item.substr(0, item.size() - 1); // Drop the last character which is the hardening marker
        }

        // Ensure this is only numbers
        const auto number{ToIntegral<uint32_t>(item)};
        if (!number) {
            return false;
        }
        path |= *number;

        parsed_keypath.push_back(path);
        first = false;
    }
    keypath = std::move(parsed_keypath);
    return true;
}

std::string FormatHDKeypath(const std::vector<uint32_t>& path, bool apostrophe)
{
    std::string ret;
    for (const uint32_t i : path) {
        const uint32_t index{i & BIP32_NON_HARDENED_KEY_MASK};
        const bool hardened{(i & BIP32_HARDENED_KEY_LIMIT) != 0};
        Assume((index | (hardened ? BIP32_HARDENED_KEY_LIMIT : 0)) == i);
        ret += strprintf("/%i", index);
        if (hardened) ret += apostrophe ? '\'' : 'h';
    }
    Assume(path.empty() == ret.empty());
    Assume(ret.empty() || ret.front() == '/');
    return ret;
}

std::string WriteHDKeypath(const std::vector<uint32_t>& keypath, bool apostrophe)
{
    const std::string formatted_path{FormatHDKeypath(keypath, apostrophe)};
    Assume(formatted_path.empty() || formatted_path.front() == '/');
    return "m" + formatted_path;
}
