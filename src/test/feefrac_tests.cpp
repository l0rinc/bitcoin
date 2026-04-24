// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/feefrac.h>
#include <random.h>

#include <test/util/check.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(feefrac_tests)

BOOST_AUTO_TEST_CASE(feefrac_operators)
{
    FeeFrac p1{1000, 100}, p2{500, 300};
    FeeFrac sum{1500, 400};
    FeeFrac diff{500, -200};
    FeeFrac empty{0, 0};
    FeeFrac zero_fee{0, 1}; // zero-fee allowed

    CHECK_EQUAL(zero_fee.EvaluateFeeDown(0), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeDown(0))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeDown(1), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeDown(1))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeDown(1000000), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeDown(1000000))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeDown(0x7fffffff), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeDown(0x7fffffff))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeUp(0), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeUp(0))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeUp(1), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeUp(1))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeUp(1000000), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeUp(1000000))>{0});
    CHECK_EQUAL(zero_fee.EvaluateFeeUp(0x7fffffff), std::remove_cvref_t<decltype(zero_fee.EvaluateFeeUp(0x7fffffff))>{0});

    CHECK_EQUAL(p1.EvaluateFeeDown(0), std::remove_cvref_t<decltype(p1.EvaluateFeeDown(0))>{0});
    CHECK_EQUAL(p1.EvaluateFeeDown(1), std::remove_cvref_t<decltype(p1.EvaluateFeeDown(1))>{10});
    CHECK_EQUAL(p1.EvaluateFeeDown(100000000), std::remove_cvref_t<decltype(p1.EvaluateFeeDown(100000000))>{1000000000});
    CHECK_EQUAL(p1.EvaluateFeeDown(0x7fffffff), int64_t(0x7fffffff) * 10);
    CHECK_EQUAL(p1.EvaluateFeeUp(0), std::remove_cvref_t<decltype(p1.EvaluateFeeUp(0))>{0});
    CHECK_EQUAL(p1.EvaluateFeeUp(1), std::remove_cvref_t<decltype(p1.EvaluateFeeUp(1))>{10});
    CHECK_EQUAL(p1.EvaluateFeeUp(100000000), std::remove_cvref_t<decltype(p1.EvaluateFeeUp(100000000))>{1000000000});
    CHECK_EQUAL(p1.EvaluateFeeUp(0x7fffffff), int64_t(0x7fffffff) * 10);

    FeeFrac neg{-1001, 100};
    CHECK_EQUAL(neg.EvaluateFeeDown(0), std::remove_cvref_t<decltype(neg.EvaluateFeeDown(0))>{0});
    CHECK_EQUAL(neg.EvaluateFeeDown(1), -11);
    CHECK_EQUAL(neg.EvaluateFeeDown(2), -21);
    CHECK_EQUAL(neg.EvaluateFeeDown(3), -31);
    CHECK_EQUAL(neg.EvaluateFeeDown(100), -1001);
    CHECK_EQUAL(neg.EvaluateFeeDown(101), -1012);
    CHECK_EQUAL(neg.EvaluateFeeDown(100000000), -1001000000);
    CHECK_EQUAL(neg.EvaluateFeeDown(100000001), -1001000011);
    CHECK_EQUAL(neg.EvaluateFeeDown(0x7fffffff), -21496311307);
    CHECK_EQUAL(neg.EvaluateFeeUp(0), std::remove_cvref_t<decltype(neg.EvaluateFeeUp(0))>{0});
    CHECK_EQUAL(neg.EvaluateFeeUp(1), -10);
    CHECK_EQUAL(neg.EvaluateFeeUp(2), -20);
    CHECK_EQUAL(neg.EvaluateFeeUp(3), -30);
    CHECK_EQUAL(neg.EvaluateFeeUp(100), -1001);
    CHECK_EQUAL(neg.EvaluateFeeUp(101), -1011);
    CHECK_EQUAL(neg.EvaluateFeeUp(100000000), -1001000000);
    CHECK_EQUAL(neg.EvaluateFeeUp(100000001), -1001000010);
    CHECK_EQUAL(neg.EvaluateFeeUp(0x7fffffff), -21496311306);

    CHECK(empty == FeeFrac{}); // same as no-args

    CHECK(p1 == p1);
    CHECK(p1 + p2 == sum);
    CHECK(p1 - p2 == diff);

    FeeFrac p3{2000, 200};
    CHECK(p1 != p3); // feefracs only equal if both fee and size are same
    CHECK(p2 != p3);

    FeeFrac p4{3000, 300};
    CHECK(p1 == p4-p3);
    CHECK(p1 + p3 == p4);

    // Fee-rate comparison
    CHECK(p1 > p2);
    CHECK(p1 >= p2);
    CHECK(p1 >= p4-p3);
    CHECK(!(p1 >> p3)); // not strictly better
    CHECK(p1 >> p2); // strictly greater feerate

    CHECK(p2 < p1);
    CHECK(p2 <= p1);
    CHECK(p1 <= p4-p3);
    CHECK(!(p3 << p1)); // not strictly worse
    CHECK(p2 << p1); // strictly lower feerate

    // "empty" comparisons
    CHECK(!(p1 >> empty)); // << will always result in false
    CHECK(!(p1 << empty));
    CHECK(!(empty >> empty));
    CHECK(!(empty << empty));

    // empty is always bigger than everything else
    CHECK(empty > p1);
    CHECK(empty > p2);
    CHECK(empty > p3);
    CHECK(empty >= p1);
    CHECK(empty >= p2);
    CHECK(empty >= p3);

    // check "max" values for comparison
    FeeFrac oversized_1{4611686000000, 4000000};
    FeeFrac oversized_2{184467440000000, 100000};

    CHECK(oversized_1 < oversized_2);
    CHECK(oversized_1 <= oversized_2);
    CHECK(oversized_1 << oversized_2);
    CHECK(oversized_1 != oversized_2);

    CHECK_EQUAL(oversized_1.EvaluateFeeDown(0), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeDown(0))>{0});
    CHECK_EQUAL(oversized_1.EvaluateFeeDown(1), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeDown(1))>{1152921});
    CHECK_EQUAL(oversized_1.EvaluateFeeDown(2), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeDown(2))>{2305843});
    CHECK_EQUAL(oversized_1.EvaluateFeeDown(1548031267), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeDown(1548031267))>{1784758530396540});
    CHECK_EQUAL(oversized_1.EvaluateFeeUp(0), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeUp(0))>{0});
    CHECK_EQUAL(oversized_1.EvaluateFeeUp(1), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeUp(1))>{1152922});
    CHECK_EQUAL(oversized_1.EvaluateFeeUp(2), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeUp(2))>{2305843});
    CHECK_EQUAL(oversized_1.EvaluateFeeUp(1548031267), std::remove_cvref_t<decltype(oversized_1.EvaluateFeeUp(1548031267))>{1784758530396541});

    // Test cases on the threshold where FeeFrac::Evaluate start using Mul/Div.
    CHECK_EQUAL(FeeFrac(0x1ffffffff, 123456789).EvaluateFeeDown(98765432), std::remove_cvref_t<decltype(FeeFrac(0x1ffffffff, 123456789).EvaluateFeeDown(98765432))>{6871947728});
    CHECK_EQUAL(FeeFrac(0x200000000, 123456789).EvaluateFeeDown(98765432), std::remove_cvref_t<decltype(FeeFrac(0x200000000, 123456789).EvaluateFeeDown(98765432))>{6871947729});
    CHECK_EQUAL(FeeFrac(0x200000001, 123456789).EvaluateFeeDown(98765432), std::remove_cvref_t<decltype(FeeFrac(0x200000001, 123456789).EvaluateFeeDown(98765432))>{6871947730});
    CHECK_EQUAL(FeeFrac(0x1ffffffff, 123456789).EvaluateFeeUp(98765432), std::remove_cvref_t<decltype(FeeFrac(0x1ffffffff, 123456789).EvaluateFeeUp(98765432))>{6871947729});
    CHECK_EQUAL(FeeFrac(0x200000000, 123456789).EvaluateFeeUp(98765432), std::remove_cvref_t<decltype(FeeFrac(0x200000000, 123456789).EvaluateFeeUp(98765432))>{6871947730});
    CHECK_EQUAL(FeeFrac(0x200000001, 123456789).EvaluateFeeUp(98765432), std::remove_cvref_t<decltype(FeeFrac(0x200000001, 123456789).EvaluateFeeUp(98765432))>{6871947731});

    // Tests paths that use double arithmetic
    FeeFrac busted{(static_cast<int64_t>(INT32_MAX)) + 1, INT32_MAX};
    CHECK(!(busted < busted));

    FeeFrac max_fee{2100000000000000, INT32_MAX};
    CHECK(!(max_fee < max_fee));
    CHECK(!(max_fee > max_fee));
    CHECK(max_fee <= max_fee);
    CHECK(max_fee >= max_fee);

    CHECK_EQUAL(max_fee.EvaluateFeeDown(0), std::remove_cvref_t<decltype(max_fee.EvaluateFeeDown(0))>{0});
    CHECK_EQUAL(max_fee.EvaluateFeeDown(1), std::remove_cvref_t<decltype(max_fee.EvaluateFeeDown(1))>{977888});
    CHECK_EQUAL(max_fee.EvaluateFeeDown(2), std::remove_cvref_t<decltype(max_fee.EvaluateFeeDown(2))>{1955777});
    CHECK_EQUAL(max_fee.EvaluateFeeDown(3), std::remove_cvref_t<decltype(max_fee.EvaluateFeeDown(3))>{2933666});
    CHECK_EQUAL(max_fee.EvaluateFeeDown(1256796054), std::remove_cvref_t<decltype(max_fee.EvaluateFeeDown(1256796054))>{1229006664189047});
    CHECK_EQUAL(max_fee.EvaluateFeeDown(INT32_MAX), std::remove_cvref_t<decltype(max_fee.EvaluateFeeDown(INT32_MAX))>{2100000000000000});
    CHECK_EQUAL(max_fee.EvaluateFeeUp(0), std::remove_cvref_t<decltype(max_fee.EvaluateFeeUp(0))>{0});
    CHECK_EQUAL(max_fee.EvaluateFeeUp(1), std::remove_cvref_t<decltype(max_fee.EvaluateFeeUp(1))>{977889});
    CHECK_EQUAL(max_fee.EvaluateFeeUp(2), std::remove_cvref_t<decltype(max_fee.EvaluateFeeUp(2))>{1955778});
    CHECK_EQUAL(max_fee.EvaluateFeeUp(3), std::remove_cvref_t<decltype(max_fee.EvaluateFeeUp(3))>{2933667});
    CHECK_EQUAL(max_fee.EvaluateFeeUp(1256796054), std::remove_cvref_t<decltype(max_fee.EvaluateFeeUp(1256796054))>{1229006664189048});
    CHECK_EQUAL(max_fee.EvaluateFeeUp(INT32_MAX), std::remove_cvref_t<decltype(max_fee.EvaluateFeeUp(INT32_MAX))>{2100000000000000});

    FeeFrac max_fee2{1, 1};
    CHECK(max_fee >= max_fee2);

    // Test for integer overflow issue (https://github.com/bitcoin/bitcoin/issues/32294)
    CHECK_EQUAL((FeeFrac{0x7ffffffdfffffffb, 0x7ffffffd}.EvaluateFeeDown(0x7fffffff)), std::remove_cvref_t<decltype((FeeFrac{0x7ffffffdfffffffb, 0x7ffffffd}.EvaluateFeeDown(0x7fffffff)))>{0x7fffffffffffffff});
}

BOOST_AUTO_TEST_SUITE_END()
