// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <rpc/client.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>

#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
bool IsJsonIntegerOutOfRange(const std::runtime_error& e)
{
    return std::string_view{e.what()} == "JSON integer out of range";
}

bool EqualUniValueState(const UniValue& a, const UniValue& b)
{
    if (a.getType() != b.getType()) return false;
    if (a.getValStr() != b.getValStr()) return false;
    if (a.getType() == UniValue::VARR || a.getType() == UniValue::VOBJ) {
        if (a.getValues().size() != b.getValues().size()) return false;
        if (a.getType() == UniValue::VOBJ && a.getKeys() != b.getKeys()) return false;
        for (size_t i{0}; i < a.getValues().size(); ++i) {
            if (!EqualUniValueState(a.getValues()[i], b.getValues()[i])) return false;
        }
    }
    return true;
}
} // namespace

void initialize_parse_univalue()
{
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(parse_univalue, .init = initialize_parse_univalue)
{
    const std::string random_string(buffer.begin(), buffer.end());
    UniValue fresh;
    UniValue preloaded{UniValue::VOBJ};
    preloaded.pushKV("sentinel", UniValue{"before"});
    preloaded.pushKV("array", UniValue{UniValue::VARR});

    const bool valid{fresh.read(random_string)};
    const bool preloaded_valid{preloaded.read(random_string)};
    assert(valid == preloaded_valid);
    assert(EqualUniValueState(fresh, preloaded));

    const UniValue& univalue{fresh};
    if (!valid) {
        return;
    }
    try {
        (void)ParseHashO(univalue, "A");
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    }
    try {
        (void)ParseHashO(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    }
    try {
        (void)ParseHashV(univalue, "A");
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    }
    try {
        (void)ParseHashV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    }
    try {
        (void)ParseHexO(univalue, "A");
    } catch (const UniValue&) {
    }
    try {
        (void)ParseHexO(univalue, random_string);
    } catch (const UniValue&) {
    }
    try {
        (void)ParseHexV(univalue, "A");
    } catch (const UniValue&) {
    }
    try {
        (void)ParseHexV(univalue, random_string);
    } catch (const UniValue&) {
    }
    try {
        if (univalue.isNull() || univalue.isStr()) (void)ParseSighashString(univalue);
    } catch (const UniValue&) {
    }
    try {
        (void)AmountFromValue(univalue);
    } catch (const UniValue&) {
    }
    try {
        FlatSigningProvider provider;
        if (buffer.size() < 10'000) (void)EvalDescriptorStringOrObject(univalue, provider);
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    } catch (const std::runtime_error& e) {
        assert(IsJsonIntegerOutOfRange(e));
    }
    try {
        (void)ParseConfirmTarget(univalue, std::numeric_limits<unsigned int>::max());
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    } catch (const std::runtime_error& e) {
        assert(IsJsonIntegerOutOfRange(e));
    }
    try {
        (void)ParseDescriptorRange(univalue);
    } catch (const UniValue&) {
    } catch (const UniValue::type_error&) {
    } catch (const std::runtime_error& e) {
        assert(IsJsonIntegerOutOfRange(e));
    }
}
