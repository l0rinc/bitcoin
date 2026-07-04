// Copyright (c) 2017 Pieter Wuille
// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bech32.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

BOOST_AUTO_TEST_SUITE(bech32_tests)

namespace {

constexpr std::string_view BECH32_CHARSET{"qpzry9x8gf2tvdw0s3jn54khce6mua7l"};

char MutatedBech32Char(char c)
{
    for (const char candidate : BECH32_CHARSET) {
        if (candidate != c) return candidate;
    }
    assert(false);
    return BECH32_CHARSET.front();
}

void CheckSingleCharacterErrors(bech32::Encoding encoding)
{
    const std::string hrp{"bc"};
    const std::vector<uint8_t> values{
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23,
        24, 25, 26, 27, 28, 29, 30, 31,
    };
    const std::string encoded{bech32::Encode(encoding, hrp, values)};
    BOOST_REQUIRE(!encoded.empty());
    BOOST_REQUIRE_LE(encoded.size(), bech32::CharLimit::BECH32);

    const size_t separator_pos{encoded.rfind(bech32::SEPARATOR)};
    BOOST_REQUIRE_NE(separator_pos, encoded.npos);

    for (size_t mutate_pos{0}; mutate_pos < encoded.size(); ++mutate_pos) {
        std::string mutated{encoded};
        mutated[mutate_pos] = MutatedBech32Char(mutated[mutate_pos]);

        const auto decoded{bech32::Decode(mutated)};
        BOOST_CHECK(decoded.encoding == bech32::Encoding::INVALID);
        BOOST_CHECK(decoded.hrp.empty());
        BOOST_CHECK(decoded.data.empty());

        const auto [error, error_locations]{bech32::LocateErrors(mutated)};
        BOOST_CHECK(!error.empty());
        if (mutate_pos > separator_pos) {
            const std::vector<int> expected_locations{static_cast<int>(mutate_pos)};
            BOOST_CHECK(error_locations == expected_locations);
        }
    }
}

} // namespace

BOOST_AUTO_TEST_CASE(bech32_testvectors_valid)
{
    static const std::string CASES[] = {
        "A12UEL5L",
        "a12uel5l",
        "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs",
        "abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw",
        "11qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqc8247j",
        "split1checkupstagehandshakeupstreamerranterredcaperred2y9e3w",
        "?1ezyfcl",
    };
    for (const std::string& str : CASES) {
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::BECH32);
        std::string recode = bech32::Encode(bech32::Encoding::BECH32, dec.hrp, dec.data);
        BOOST_CHECK(!recode.empty());
        BOOST_CHECK(CaseInsensitiveEqual(str, recode));
        auto [error, error_locations] = bech32::LocateErrors(str);
        BOOST_CHECK(error.empty());
        BOOST_CHECK(error_locations.empty());
    }
}

BOOST_AUTO_TEST_CASE(bech32m_testvectors_valid)
{
    static const std::string CASES[] = {
        "A1LQFN3A",
        "a1lqfn3a",
        "an83characterlonghumanreadablepartthatcontainsthetheexcludedcharactersbioandnumber11sg7hg6",
        "abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx",
        "11llllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllllludsr8",
        "split1checkupstagehandshakeupstreamerranterredcaperredlc445v",
        "?1v759aa"
    };
    for (const std::string& str : CASES) {
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::BECH32M);
        std::string recode = bech32::Encode(bech32::Encoding::BECH32M, dec.hrp, dec.data);
        BOOST_CHECK(!recode.empty());
        BOOST_CHECK(CaseInsensitiveEqual(str, recode));
        auto [error, error_locations] = bech32::LocateErrors(str);
        BOOST_CHECK(error.empty());
        BOOST_CHECK(error_locations.empty());
    }
}

BOOST_AUTO_TEST_CASE(bech32_decode_normalizes_uppercase)
{
    const std::vector<uint8_t> values{0, 1, 2, 3, 4, 5, 30, 31};

    for (const auto encoding : {bech32::Encoding::BECH32, bech32::Encoding::BECH32M}) {
        const std::string lowercase{bech32::Encode(encoding, "bc", values)};
        const std::string encoded{ToUpper(lowercase)};
        const auto decoded{bech32::Decode(encoded)};
        BOOST_REQUIRE(decoded.encoding == encoding);
        BOOST_CHECK_EQUAL(decoded.hrp, "bc");
        BOOST_CHECK(decoded.data == values);

        const std::string reencoded{bech32::Encode(decoded.encoding, decoded.hrp, decoded.data)};
        BOOST_CHECK_EQUAL(reencoded, lowercase);
    }
}

BOOST_AUTO_TEST_CASE(bech32_testvectors_invalid)
{
    static const std::string CASES[] = {
        " 1nwldj5",
        "\x7f""1axkwrx",
        "\x80""1eym55h",
        "an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1569pvx",
        "pzry9x0s0muk",
        "1pzry9x0s0muk",
        "x1b4n0q5v",
        "li1dgmt3",
        "de1lg7wt\xff",
        "A1G7SGD8",
        "10a06t8",
        "1qzzfhee",
        "a12UEL5L",
        "A12uEL5L",
        "abcdef1qpzrz9x8gf2tvdw0s3jn54khce6mua7lmqqqxw",
        "test1zg69w7y6hn0aqy352euf40x77qddq3dc",
    };
    static const std::pair<std::string, std::vector<int>> ERRORS[] = {
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Bech32 string too long", {90}},
        {"Missing separator", {}},
        {"Invalid separator position", {0}},
        {"Invalid Base 32 character", {2}},
        {"Invalid separator position", {2}},
        {"Invalid character or mixed case", {8}},
        {"Invalid checksum", {}}, // The checksum is calculated using the uppercase form so the entire string is invalid, not just a few characters
        {"Invalid separator position", {0}},
        {"Invalid separator position", {0}},
        {"Invalid character or mixed case", {3, 4, 5, 7}},
        {"Invalid character or mixed case", {3}},
        {"Invalid Bech32 checksum", {11}},
        {"Invalid Bech32 checksum", {9, 16}},
    };
    static_assert(std::size(CASES) == std::size(ERRORS), "Bech32 CASES and ERRORS should have the same length");

    int i = 0;
    for (const std::string& str : CASES) {
        const auto& err = ERRORS[i];
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
        auto [error, error_locations] = bech32::LocateErrors(str);
        BOOST_CHECK_EQUAL(err.first, error);
        BOOST_CHECK(err.second == error_locations);
        i++;
    }
}

BOOST_AUTO_TEST_CASE(bech32m_testvectors_invalid)
{
    static const std::string CASES[] = {
        " 1xj0phk",
        "\x7f""1g6xzxy",
        "\x80""1vctc34",
        "an84characterslonghumanreadablepartthatcontainsthetheexcludedcharactersbioandnumber11d6pts4",
        "qyrz8wqd2c9m",
        "1qyrz8wqd2c9m",
        "y1b0jsk6g",
        "lt1igcx5c0",
        "in1muywd",
        "mm1crxm3i",
        "au1s5cgom",
        "M1VUXWEZ",
        "16plkw9",
        "1p2gdwpf",
        "abcdef1l7aum6echk45nj2s0wdvt2fg8x9yrzpqzd3ryx",
        "test1zg69v7y60n00qy352euf40x77qcusag6",
    };
    static const std::pair<std::string, std::vector<int>> ERRORS[] = {
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Invalid character or mixed case", {0}},
        {"Bech32 string too long", {90}},
        {"Missing separator", {}},
        {"Invalid separator position", {0}},
        {"Invalid Base 32 character", {2}},
        {"Invalid Base 32 character", {3}},
        {"Invalid separator position", {2}},
        {"Invalid Base 32 character", {8}},
        {"Invalid Base 32 character", {7}},
        {"Invalid checksum", {}},
        {"Invalid separator position", {0}},
        {"Invalid separator position", {0}},
        {"Invalid Bech32m checksum", {21}},
        {"Invalid Bech32m checksum", {13, 32}},
    };
    static_assert(std::size(CASES) == std::size(ERRORS), "Bech32m CASES and ERRORS should have the same length");

    int i = 0;
    for (const std::string& str : CASES) {
        const auto& err = ERRORS[i];
        const auto dec = bech32::Decode(str);
        BOOST_CHECK(dec.encoding == bech32::Encoding::INVALID);
        auto [error, error_locations] = bech32::LocateErrors(str);
        BOOST_CHECK_EQUAL(err.first, error);
        BOOST_CHECK(err.second == error_locations);
        i++;
    }
}

BOOST_AUTO_TEST_CASE(bech32_single_character_errors)
{
    CheckSingleCharacterErrors(bech32::Encoding::BECH32);
    CheckSingleCharacterErrors(bech32::Encoding::BECH32M);
}

BOOST_AUTO_TEST_SUITE_END()
