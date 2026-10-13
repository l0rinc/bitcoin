// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <span>

BOOST_FIXTURE_TEST_SUITE(script_segwit_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(IsPayToWitnessScriptHash_Valid)
{
    uint256 dummy;
    CScript p2wsh;
    p2wsh << OP_0 << ToByteVector(dummy);
    BOOST_CHECK(p2wsh.IsPayToWitnessScriptHash());

    std::vector<unsigned char> bytes = {OP_0, 32};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(CScript(bytes.begin(), bytes.end()).IsPayToWitnessScriptHash());
}

BOOST_AUTO_TEST_CASE(IsPayToWitnessScriptHash_Invalid_NotOp0)
{
    uint256 dummy;
    CScript notp2wsh;
    notp2wsh << OP_1 << ToByteVector(dummy);
    BOOST_CHECK(!notp2wsh.IsPayToWitnessScriptHash());
}

BOOST_AUTO_TEST_CASE(IsPayToWitnessScriptHash_Invalid_Size)
{
    uint160 dummy;
    CScript notp2wsh;
    notp2wsh << OP_0 << ToByteVector(dummy);
    BOOST_CHECK(!notp2wsh.IsPayToWitnessScriptHash());
}

BOOST_AUTO_TEST_CASE(IsPayToWitnessScriptHash_Invalid_Nop)
{
    uint256 dummy;
    CScript notp2wsh;
    notp2wsh << OP_0 << OP_NOP << ToByteVector(dummy);
    BOOST_CHECK(!notp2wsh.IsPayToWitnessScriptHash());
}

BOOST_AUTO_TEST_CASE(IsPayToWitnessScriptHash_Invalid_EmptyScript)
{
    CScript notp2wsh;
    BOOST_CHECK(!notp2wsh.IsPayToWitnessScriptHash());
}

BOOST_AUTO_TEST_CASE(IsPayToWitnessScriptHash_Invalid_Pushdata)
{
    // A script is not P2WSH if OP_PUSHDATA is used to push the hash.
    std::vector<unsigned char> bytes = {OP_0, OP_PUSHDATA1, 32};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(!CScript(bytes.begin(), bytes.end()).IsPayToWitnessScriptHash());

    bytes = {OP_0, OP_PUSHDATA2, 32, 0};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(!CScript(bytes.begin(), bytes.end()).IsPayToWitnessScriptHash());

    bytes = {OP_0, OP_PUSHDATA4, 32, 0, 0, 0};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(!CScript(bytes.begin(), bytes.end()).IsPayToWitnessScriptHash());
}

namespace {

bool IsExpectedWitnessProgram(const CScript& script, const int expectedVersion, const std::vector<unsigned char>& expectedProgram)
{
    int actualVersion;
    std::vector<unsigned char> actualProgram;
    if (!script.IsWitnessProgram(actualVersion, actualProgram)) {
        return false;
    }
    BOOST_CHECK_EQUAL(actualVersion, expectedVersion);
    BOOST_CHECK(actualProgram == expectedProgram);
    int view_version{-1};
    std::span<const unsigned char> view;
    BOOST_REQUIRE(script.IsWitnessProgram(view_version, view));
    BOOST_CHECK_EQUAL(view_version, expectedVersion);
    BOOST_CHECK(std::ranges::equal(view, expectedProgram));
    BOOST_CHECK(view.data() == script.data() + 2);
    return true;
}

bool IsNoWitnessProgram(const CScript& script)
{
    int version{-1};
    std::vector<unsigned char> program{42, 18};
    const bool recognized{script.IsWitnessProgram(version, program)};
    BOOST_CHECK_EQUAL(version, -1);
    BOOST_CHECK(program == std::vector<unsigned char>({42, 18}));
    std::span<const unsigned char> view{program};
    const auto* original_data{view.data()};
    BOOST_CHECK(!script.IsWitnessProgram(version, view));
    BOOST_CHECK_EQUAL(version, -1);
    BOOST_CHECK(view.data() == original_data);
    BOOST_CHECK_EQUAL(view.size(), program.size());
    return !recognized;
}

} // anonymous namespace

BOOST_AUTO_TEST_CASE(IsWitnessProgram_Valid)
{
    // Witness programs have a minimum data push of 2 bytes.
    std::vector<unsigned char> program = {42, 18};
    CScript wit;
    wit << OP_0 << program;
    BOOST_CHECK(IsExpectedWitnessProgram(wit, 0, program));

    wit.clear();
    // Witness programs have a maximum data push of 40 bytes.
    program.resize(40);
    wit << OP_16 << program;
    BOOST_CHECK(IsExpectedWitnessProgram(wit, 16, program));

    program.resize(32);
    std::vector<unsigned char> bytes = {OP_5, static_cast<unsigned char>(program.size())};
    bytes.insert(bytes.end(), program.begin(), program.end());
    BOOST_CHECK(IsExpectedWitnessProgram(CScript(bytes.begin(), bytes.end()), 5, program));
}

BOOST_AUTO_TEST_CASE(IsWitnessProgram_Invalid_Version)
{
    std::vector<unsigned char> program(10);
    CScript nowit;
    nowit << OP_1NEGATE << program;
    BOOST_CHECK(IsNoWitnessProgram(nowit));
}

BOOST_AUTO_TEST_CASE(IsWitnessProgram_AllVersionsAndLengths)
{
    for (int version{0}; version <= 16; ++version) {
        for (size_t size{2}; size <= 40; ++size) {
            std::vector<unsigned char> program(size);
            for (size_t i{0}; i < size; ++i) program[i] = static_cast<unsigned char>(i + version);
            const CScript script{CScript{} << CScript::EncodeOP_N(version) << program};
            BOOST_CHECK(IsExpectedWitnessProgram(script, version, program));
        }
    }
}

BOOST_AUTO_TEST_CASE(IsWitnessProgram_Invalid_Size)
{
    std::vector<unsigned char> program(1);
    CScript nowit;
    nowit << OP_0 << program;
    BOOST_CHECK(IsNoWitnessProgram(nowit));

    nowit.clear();
    program.resize(41);
    nowit << OP_0 << program;
    BOOST_CHECK(IsNoWitnessProgram(nowit));
}

BOOST_AUTO_TEST_CASE(IsWitnessProgram_Invalid_Nop)
{
    std::vector<unsigned char> program(10);
    CScript nowit;
    nowit << OP_0 << OP_NOP << program;
    BOOST_CHECK(IsNoWitnessProgram(nowit));
}

BOOST_AUTO_TEST_CASE(IsWitnessProgram_Invalid_EmptyScript)
{
    CScript nowit;
    BOOST_CHECK(IsNoWitnessProgram(nowit));
}

BOOST_AUTO_TEST_CASE(IsWitnessProgram_Invalid_Pushdata)
{
    // A script is no witness program if OP_PUSHDATA is used to push the hash.
    std::vector<unsigned char> bytes = {OP_0, OP_PUSHDATA1, 32};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(IsNoWitnessProgram(CScript(bytes.begin(), bytes.end())));

    bytes = {OP_0, OP_PUSHDATA2, 32, 0};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(IsNoWitnessProgram(CScript(bytes.begin(), bytes.end())));

    bytes = {OP_0, OP_PUSHDATA4, 32, 0, 0, 0};
    bytes.insert(bytes.end(), 32, 0);
    BOOST_CHECK(IsNoWitnessProgram(CScript(bytes.begin(), bytes.end())));
}

BOOST_AUTO_TEST_SUITE_END()
