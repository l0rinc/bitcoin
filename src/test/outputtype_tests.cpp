// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <outputtype.h>

#include <string>
#include <string_view>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(outputtype_tests)

namespace {
void CheckOutputTypeRoundTrip(const OutputType output_type, const std::string_view name)
{
    const std::optional<OutputType> parsed_type{ParseOutputType(name)};
    BOOST_REQUIRE(parsed_type);
    BOOST_CHECK(*parsed_type == output_type);
    BOOST_CHECK_EQUAL(FormatOutputType(output_type), std::string{name});
    BOOST_CHECK(ParseOutputType(FormatOutputType(output_type)) == output_type);
}
} // namespace

BOOST_AUTO_TEST_CASE(parse_output_type)
{
    CheckOutputTypeRoundTrip(OutputType::LEGACY, "legacy");
    CheckOutputTypeRoundTrip(OutputType::P2SH_SEGWIT, "p2sh-segwit");
    CheckOutputTypeRoundTrip(OutputType::BECH32, "bech32");
    CheckOutputTypeRoundTrip(OutputType::BECH32M, "bech32m");

    BOOST_CHECK(!ParseOutputType(""));
    BOOST_CHECK(!ParseOutputType("unknown"));
    BOOST_CHECK(!ParseOutputType("LEGACY"));
    BOOST_CHECK(!ParseOutputType("bech32 "));
    BOOST_CHECK(!ParseOutputType("bech32-m"));
    BOOST_CHECK(!ParseOutputType(std::string_view{"legacy\0", 7}));

    BOOST_CHECK_EQUAL(FormatOutputType(OutputType::UNKNOWN), "unknown");
    BOOST_CHECK(!ParseOutputType(FormatOutputType(OutputType::UNKNOWN)));
    BOOST_CHECK_EQUAL(FormatAllOutputTypes(), "\"legacy\", \"p2sh-segwit\", \"bech32\", \"bech32m\"");
}

BOOST_AUTO_TEST_SUITE_END()
