// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/amount.h>
#include <rpc/client.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
bool IsJsonIntegerOutOfRange(const std::runtime_error& e)
{
    return std::string_view{e.what()} == "JSON integer out of range";
}

void AssertParsedHash(const uint256& parsed, const UniValue& value)
{
    const std::string& hex{value.get_str()};
    const auto direct{uint256::FromHex(hex)};
    assert(direct);
    assert(parsed == *direct);
    assert(parsed.GetHex() == ToLower(hex));
}

void AssertParsedHex(const std::vector<unsigned char>& parsed, const UniValue& value)
{
    const std::string& hex{value.get_str()};
    assert(IsHex(hex));
    assert(parsed == ParseHex(hex));
    assert(HexStr(parsed) == ToLower(hex));
}

void AssertParsedAmount(const CAmount amount, const UniValue& value)
{
    int64_t parsed{0};
    assert(ParseFixedPoint(value.getValStr(), 8, &parsed));
    assert(amount == parsed);
    assert(MoneyRange(amount));
}

void AssertParsedDescriptorRange(const std::pair<int64_t, int64_t>& range)
{
    assert(range.first >= 0);
    assert(range.second >= range.first);
    assert((range.second >> 31) == 0);
    assert(range.second < range.first + 1'000'000);
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

void AssertJSONRPCError(const UniValue& error)
{
    assert(error.isObject());
    assert(error.size() == 2);
    assert(error.exists("code"));
    assert(error.exists("message"));
    assert(error["code"].isNum());
    assert(error["message"].isStr());
}

void AssertWriteRoundTrip(const UniValue& value)
{
    const std::string encoded{value.write()};
    UniValue reparsed;
    assert(reparsed.read(encoded));
    assert(EqualUniValueState(value, reparsed));
    assert(reparsed.write() == encoded);
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
    AssertWriteRoundTrip(univalue);
    try {
        AssertParsedHash(ParseHashO(univalue, "A"), univalue.find_value("A"));
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    }
    try {
        AssertParsedHash(ParseHashO(univalue, random_string), univalue.find_value(random_string));
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    }
    try {
        AssertParsedHash(ParseHashV(univalue, "A"), univalue);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    }
    try {
        AssertParsedHash(ParseHashV(univalue, random_string), univalue);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    }
    try {
        AssertParsedHex(ParseHexO(univalue, "A"), univalue.find_value("A"));
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    }
    try {
        AssertParsedHex(ParseHexO(univalue, random_string), univalue.find_value(random_string));
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    }
    try {
        AssertParsedHex(ParseHexV(univalue, "A"), univalue);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    }
    try {
        AssertParsedHex(ParseHexV(univalue, random_string), univalue);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    }
    try {
        if (univalue.isNull() || univalue.isStr()) (void)ParseSighashString(univalue);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    }
    try {
        AssertParsedAmount(AmountFromValue(univalue), univalue);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    }
    try {
        FlatSigningProvider provider;
        if (buffer.size() < 10'000) (void)EvalDescriptorStringOrObject(univalue, provider);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    } catch (const std::runtime_error& e) {
        assert(IsJsonIntegerOutOfRange(e));
    }
    try {
        const unsigned int confirm_target{ParseConfirmTarget(univalue, std::numeric_limits<unsigned int>::max())};
        assert(confirm_target >= 1);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    } catch (const std::runtime_error& e) {
        assert(IsJsonIntegerOutOfRange(e));
    }
    try {
        AssertParsedDescriptorRange(ParseDescriptorRange(univalue));
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
    } catch (const UniValue::type_error&) {
    } catch (const std::runtime_error& e) {
        assert(IsJsonIntegerOutOfRange(e));
    }
}
