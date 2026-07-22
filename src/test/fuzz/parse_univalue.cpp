// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <rpc/client.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>

#include <cassert>
#include <limits>
#include <string>

void initialize_parse_univalue()
{
    SelectParams(ChainType::REGTEST);
}

// Keep the parser/writer round trip independent of UniValue's implementation.
static void AssertEquivalent(const UniValue& expected, const UniValue& actual)
{
    assert(expected.getType() == actual.getType());
    if (expected.isObject()) {
        assert(expected.getKeys() == actual.getKeys());
        assert(expected.size() == actual.size());
        const auto& expected_values{expected.getValues()};
        const auto& actual_values{actual.getValues()};
        for (size_t i{0}; i < expected_values.size(); ++i) {
            AssertEquivalent(expected_values[i], actual_values[i]);
        }
        return;
    }
    if (expected.isArray()) {
        assert(expected.size() == actual.size());
        const auto& expected_values{expected.getValues()};
        const auto& actual_values{actual.getValues()};
        for (size_t i{0}; i < expected_values.size(); ++i) {
            AssertEquivalent(expected_values[i], actual_values[i]);
        }
        return;
    }
    assert(expected.getValStr() == actual.getValStr());
}

FUZZ_TARGET(parse_univalue, .init = initialize_parse_univalue)
{
    const std::string random_string(buffer.begin(), buffer.end());
    bool valid = true;
    const UniValue univalue = [&] {
        UniValue uv;
        if (!uv.read(random_string)) valid = false;
        return valid ? uv : UniValue{};
    }();
    if (!valid) {
        return;
    }

    const std::string canonical{univalue.write()};
    UniValue reparsed;
    assert(reparsed.read(canonical));
    assert(reparsed.getType() == univalue.getType());
    AssertEquivalent(univalue, reparsed);
    assert(reparsed.write() == canonical);

    try {
        (void)ParseHashO(univalue, "A");
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashO(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashV(univalue, "A");
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHashV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
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
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseHexV(univalue, random_string);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        if (univalue.isNull() || univalue.isStr()) (void)ParseSighashString(univalue);
    } catch (const UniValue&) {
    }
    try {
        (void)AmountFromValue(univalue);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        FlatSigningProvider provider;
        if (buffer.size() < 10'000) (void)EvalDescriptorStringOrObject(univalue, provider);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseConfirmTarget(univalue, std::numeric_limits<unsigned int>::max());
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
    try {
        (void)ParseDescriptorRange(univalue);
    } catch (const UniValue&) {
    } catch (const std::runtime_error&) {
    }
}
