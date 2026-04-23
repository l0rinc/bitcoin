// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <common/url.h>
#include <test/util/check.h>

#include <string>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(common_url_tests)

// These test vectors were ported from test/regress.c in the libevent library
// which used to be a dependency of the UrlDecode function.

BOOST_AUTO_TEST_CASE(encode_decode_test) {
    CHECK_EQUAL(UrlDecode("Hello"), std::string_view{"Hello"});
    CHECK_EQUAL(UrlDecode("99"), std::string_view{"99"});
    CHECK_EQUAL(UrlDecode(""), std::string_view{""});
    CHECK_EQUAL(UrlDecode("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789-.~_"), std::string_view{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789-.~_"});
    CHECK_EQUAL(UrlDecode("%20"), std::string_view{" "});
    CHECK_EQUAL(UrlDecode("%FF%F0%E0"), std::string_view{"\xff\xf0\xe0"});
    CHECK_EQUAL(UrlDecode("%01%19"), std::string_view{"\x01\x19"});
    CHECK_EQUAL(UrlDecode("http%3A%2F%2Fwww.ietf.org%2Frfc%2Frfc3986.txt"), std::string_view{"http://www.ietf.org/rfc/rfc3986.txt"});
    CHECK_EQUAL(UrlDecode("1%2B2%3D3"), std::string_view{"1+2=3"});
}

BOOST_AUTO_TEST_CASE(decode_malformed_test) {
    CHECK_EQUAL(UrlDecode("%%xhello th+ere \xff"), std::string_view{"%%xhello th+ere \xff"});

    CHECK_EQUAL(UrlDecode("%"), std::string_view{"%"});
    CHECK_EQUAL(UrlDecode("%%"), std::string_view{"%%"});
    CHECK_EQUAL(UrlDecode("%%%"), std::string_view{"%%%"});
    CHECK_EQUAL(UrlDecode("%%%%"), std::string_view{"%%%%"});

    CHECK_EQUAL(UrlDecode("+"), std::string_view{"+"});
    CHECK_EQUAL(UrlDecode("++"), std::string_view{"++"});

    CHECK_EQUAL(UrlDecode("?"), std::string_view{"?"});
    CHECK_EQUAL(UrlDecode("??"), std::string_view{"??"});

    CHECK_EQUAL(UrlDecode("%G1"), std::string_view{"%G1"});
    CHECK_EQUAL(UrlDecode("%2"), std::string_view{"%2"});
    CHECK_EQUAL(UrlDecode("%ZX"), std::string_view{"%ZX"});

    CHECK_EQUAL(UrlDecode("valid%20string%G1"), std::string_view{"valid string%G1"});
    CHECK_EQUAL(UrlDecode("%20invalid%ZX"), std::string_view{" invalid%ZX"});
    CHECK_EQUAL(UrlDecode("%20%G1%ZX"), std::string_view{" %G1%ZX"});

    CHECK_EQUAL(UrlDecode("%1 "), std::string_view{"%1 "});
    CHECK_EQUAL(UrlDecode("% 9"), std::string_view{"% 9"});
    CHECK_EQUAL(UrlDecode(" %Z "), std::string_view{" %Z "});
    CHECK_EQUAL(UrlDecode(" % X"), std::string_view{" % X"});

    CHECK_EQUAL(UrlDecode("%%ffg"), std::string_view{"%\xffg"});
    CHECK_EQUAL(UrlDecode("%fg"), std::string_view{"%fg"});

    CHECK_EQUAL(UrlDecode("%-1"), std::string_view{"%-1"});
    CHECK_EQUAL(UrlDecode("%1-"), std::string_view{"%1-"});
}

BOOST_AUTO_TEST_CASE(decode_lowercase_hex_test) {
    CHECK_EQUAL(UrlDecode("%f0%a0%b0"), std::string_view{"\xf0\xa0\xb0"});
}

BOOST_AUTO_TEST_CASE(decode_internal_nulls_test) {
    std::string result1{"\0\0x\0\0", 5};
    CHECK_EQUAL(UrlDecode("%00%00x%00%00"), result1);
    std::string result2{"abc\0\0", 5};
    CHECK_EQUAL(UrlDecode("abc%00%00"), result2);
}

BOOST_AUTO_TEST_SUITE_END()
