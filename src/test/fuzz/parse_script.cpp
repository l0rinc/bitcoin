// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <ranges>
#include <stdexcept>
#include <string>

FUZZ_TARGET(parse_script)
{
    const std::string script_string(buffer.begin(), buffer.end());
    CScript parsed;
    try {
        parsed = ParseScript(script_string);
    } catch (const ScriptParseError&) {
        return;
    }
    const std::string formatted{FormatScript(parsed)};
    const CScript reparsed{ParseScript(formatted)};
    assert(std::ranges::equal(reparsed, parsed));
    assert(FormatScript(reparsed) == formatted);
}
