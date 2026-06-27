// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <script/script.h>
#include <util/strencodings.h>
#include <test/util/common.h>

#include <boost/test/unit_test.hpp>

#include <span>
#include <string>
#include <string_view>
#include <vector>

BOOST_AUTO_TEST_SUITE(script_parse_tests)
BOOST_AUTO_TEST_CASE(parse_script)
{
    const std::vector<std::pair<std::string,std::string>> IN_OUT{
        // {IN: script string , OUT: hex string }
        {"", ""},
        {"0", "00"},
        {"1", "51"},
        {"2", "52"},
        {"3", "53"},
        {"4", "54"},
        {"5", "55"},
        {"6", "56"},
        {"7", "57"},
        {"8", "58"},
        {"9", "59"},
        {"10", "5a"},
        {"11", "5b"},
        {"12", "5c"},
        {"13", "5d"},
        {"14", "5e"},
        {"15", "5f"},
        {"16", "60"},
        {"17", "0111"},
        {"-9", "0189"},
        {"0x17", "17"},
        {"'17'", "023137"},
        {"ELSE", "67"},
        {"NOP10", "b9"},
    };
    std::string all_in;
    std::string all_out;
    for (const auto& [in, out] : IN_OUT) {
        BOOST_CHECK_EQUAL(HexStr(ParseScript(in)), out);
        all_in += " " + in + " ";
        all_out += out;
    }
    BOOST_CHECK_EQUAL(HexStr(ParseScript(all_in)), all_out);

    BOOST_CHECK_EXCEPTION(ParseScript("11111111111111111111"), std::runtime_error, HasReason("script parse error: decimal numeric value only allowed in the range -0xFFFFFFFF...0xFFFFFFFF"));
    BOOST_CHECK_EXCEPTION(ParseScript("11111111111"), std::runtime_error, HasReason("script parse error: decimal numeric value only allowed in the range -0xFFFFFFFF...0xFFFFFFFF"));
    BOOST_CHECK_EXCEPTION(ParseScript("OP_CHECKSIGADD"), std::runtime_error, HasReason("script parse error: unknown opcode"));
}

BOOST_AUTO_TEST_CASE(format_script_round_trip)
{
    const auto raw_script{[](std::vector<unsigned char> bytes) {
        return CScript{bytes.begin(), bytes.end()};
    }};
    const auto check_round_trip{[](const CScript& script, std::string_view expected_format) {
        const std::string formatted{FormatScript(script)};
        BOOST_CHECK_EQUAL(formatted, expected_format);

        const CScript reparsed{ParseScript(formatted)};
        BOOST_CHECK_EQUAL(HexStr(reparsed), HexStr(script));
        BOOST_CHECK_EQUAL(FormatScript(reparsed), formatted);
    }};

    check_round_trip(CScript{}, "");
    check_round_trip(CScript{} << OP_0 << OP_1NEGATE << OP_1 << OP_16 << OP_NOP << OP_NOP10, "0 -1 1 16 NOP NOP10");

    const std::vector<unsigned char> data{0xab, 0xcd};
    CScript data_push;
    data_push << std::span{data};
    check_round_trip(data_push, "0x02 0xabcd");

    check_round_trip(raw_script({static_cast<unsigned char>(OP_PUSHDATA1), 0x01, 0xff}), "0x4c01 0xff");
    check_round_trip(raw_script({static_cast<unsigned char>(OP_PUSHDATA1), 0x00}), "0x4c00");
    check_round_trip(raw_script({static_cast<unsigned char>(OP_PUSHDATA1)}), "0x4c");
    check_round_trip(raw_script({static_cast<unsigned char>(OP_PUSHDATA2), 0x02}), "0x4d02");
    check_round_trip(raw_script({0xff}), "0xff");
}
BOOST_AUTO_TEST_SUITE_END()
