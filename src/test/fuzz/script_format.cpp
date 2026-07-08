// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/consensus.h>
#include <core_io.h>
#include <key_io.h>
#include <policy/policy.h>
#include <script/script.h>
#include <script/solver.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <univalue.h>
#include <util/chaintype.h>

#include <cassert>
#include <ranges>
#include <string>
#include <vector>

void initialize_script_format()
{
    SelectParams(ChainType::REGTEST);
}

namespace {
void AssertScriptToUnivContracts(const CScript& script, bool include_hex, bool include_address)
{
    UniValue out{UniValue::VOBJ};
    ScriptToUniv(script, out, include_hex, include_address);

    std::vector<std::vector<unsigned char>> solutions;
    const TxoutType type{Solver(script, solutions)};
    CTxDestination destination;
    const bool has_address{include_address && ExtractDestination(script, destination) && type != TxoutType::PUBKEY};

    assert(out.isObject());
    assert(out.exists("asm"));
    assert(out["asm"].get_str() == ScriptToAsmStr(script));
    assert(out.exists("type"));
    assert(out["type"].get_str() == GetTxnOutputType(type));
    assert(out.exists("hex") == include_hex);
    if (include_hex) {
        assert(out["hex"].get_str() == HexStr(script));
    }
    assert(out.exists("desc") == include_address);
    if (include_address) {
        assert(!out["desc"].get_str().empty());
    }
    assert(out.exists("address") == has_address);
    if (has_address) {
        assert(out["address"].get_str() == EncodeDestination(destination));
    }
    assert(out.size() == 2 + static_cast<unsigned int>(include_hex) + static_cast<unsigned int>(include_address) + static_cast<unsigned int>(has_address));
}
} // namespace

FUZZ_TARGET(script_format, .init = initialize_script_format)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CScript script{ConsumeScript(fuzzed_data_provider)};
    if (script.size() > MAX_STANDARD_TX_WEIGHT / WITNESS_SCALE_FACTOR) {
        return;
    }

    const std::string formatted{FormatScript(script)};
    const CScript reparsed{ParseScript(formatted)};
    assert(std::ranges::equal(reparsed, script));
    assert(FormatScript(reparsed) == formatted);

    (void)ScriptToAsmStr(script, /*fAttemptSighashDecode=*/fuzzed_data_provider.ConsumeBool());

    auto include_hex = fuzzed_data_provider.ConsumeBool();
    auto include_address = fuzzed_data_provider.ConsumeBool();
    AssertScriptToUnivContracts(script, include_hex, include_address);
}
