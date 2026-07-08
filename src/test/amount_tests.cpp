// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>

#include <limits>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(amount_tests)

BOOST_AUTO_TEST_CASE(MoneyRangeTest)
{
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(-1)), false);
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(0)), true);
    BOOST_CHECK_EQUAL(MoneyRange(CAmount(1)), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY), true);
    BOOST_CHECK_EQUAL(MoneyRange(MAX_MONEY + CAmount(1)), false);
}

BOOST_AUTO_TEST_CASE(GetFeeTest)
{
    CFeeRate feeRate, altFeeRate;

    feeRate = CFeeRate(0);
    // Must always return 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e5), CAmount(0));

    feeRate = CFeeRate(1000);
    // Must always just return the arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), CAmount(1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), CAmount(121));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), CAmount(999));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), CAmount(1e3));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), CAmount(9e3));

    feeRate = CFeeRate(-1000);
    // Must always just return -1 * arg
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1), CAmount(-1));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), CAmount(-121));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), CAmount(-999));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), CAmount(-1e3));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), CAmount(-9e3));

    feeRate = CFeeRate(123);
    // Rounds up the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), CAmount(1)); // Special case: returns 1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), CAmount(2));
    BOOST_CHECK_EQUAL(feeRate.GetFee(121), CAmount(15));
    BOOST_CHECK_EQUAL(feeRate.GetFee(122), CAmount(16));
    BOOST_CHECK_EQUAL(feeRate.GetFee(999), CAmount(123));
    BOOST_CHECK_EQUAL(feeRate.GetFee(1e3), CAmount(123));
    BOOST_CHECK_EQUAL(feeRate.GetFee(9e3), CAmount(1107));

    feeRate = CFeeRate(-123);
    // Truncates the result, if not integer
    BOOST_CHECK_EQUAL(feeRate.GetFee(0), CAmount(0));
    BOOST_CHECK_EQUAL(feeRate.GetFee(8), CAmount(-1)); // Special case: returns -1 instead of 0
    BOOST_CHECK_EQUAL(feeRate.GetFee(9), CAmount(-1));

    // check alternate constructor
    feeRate = CFeeRate(1000);
    altFeeRate = CFeeRate(feeRate);
    BOOST_CHECK_EQUAL(feeRate.GetFee(100), altFeeRate.GetFee(100));

    // Check full constructor
    BOOST_CHECK(CFeeRate(CAmount(-1), 0) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(0), 0) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(1), 0) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(1), -1000) == CFeeRate(0));
    // default value
    BOOST_CHECK(CFeeRate(CAmount(-1), 1000) == CFeeRate(-1));
    BOOST_CHECK(CFeeRate(CAmount(0), 1000) == CFeeRate(0));
    BOOST_CHECK(CFeeRate(CAmount(1), 1000) == CFeeRate(1));
    BOOST_CHECK_EQUAL(CFeeRate(CAmount(1), 1001).GetFee(1001), CAmount(1));
    BOOST_CHECK_EQUAL(CFeeRate(CAmount(-2), 1001).GetFee(1001), CAmount(-2));
    // Previously, precision was limited to three decimal digits
    // due to only supporting satoshis per kB, so CFeeRate(CAmount(1), 1001) was equal to CFeeRate(0)
    // Since #32750, higher precision is maintained.
    BOOST_CHECK(CFeeRate(CAmount(1), 1001) > CFeeRate(0) && CFeeRate(CAmount(1), 1001) < CFeeRate(1));
    BOOST_CHECK(CFeeRate(CAmount(2), 1001) > CFeeRate(1) && CFeeRate(CAmount(2), 1001) < CFeeRate(2));
    // some more integer checks
    BOOST_CHECK(CFeeRate(CAmount(26), 789) > CFeeRate(32) && CFeeRate(CAmount(26), 789) < CFeeRate(33));
    BOOST_CHECK(CFeeRate(CAmount(27), 789) > CFeeRate(34) && CFeeRate(CAmount(27), 789) < CFeeRate(35));
    // Maximum size in bytes, should not crash
    CFeeRate(MAX_MONEY, std::numeric_limits<int32_t>::max()).GetFeePerK();
    BOOST_CHECK_EQUAL(CFeeRate(MAX_MONEY, std::numeric_limits<int32_t>::max()).GetFee(std::numeric_limits<int32_t>::max()), MAX_MONEY);

    // check multiplication operator
    // check multiplying by zero
    feeRate = CFeeRate(1000);
    BOOST_CHECK(0 * feeRate == CFeeRate(0));
    BOOST_CHECK(feeRate * 0 == CFeeRate(0));
    // check multiplying by a positive integer
    BOOST_CHECK(3 * feeRate == CFeeRate(3000));
    BOOST_CHECK(feeRate * 3 == CFeeRate(3000));
    // check multiplying by a negative integer
    BOOST_CHECK(-3 * feeRate == CFeeRate(-3000));
    BOOST_CHECK(feeRate * -3 == CFeeRate(-3000));
    // check commutativity
    BOOST_CHECK(2 * feeRate == feeRate * 2);
    // check with large numbers
    int largeNumber = 1000000;
    BOOST_CHECK(largeNumber * feeRate == feeRate * largeNumber);
    // check boundary values
    int maxInt = std::numeric_limits<int>::max();
    feeRate = CFeeRate(maxInt);
    BOOST_CHECK(feeRate * 2 == CFeeRate(static_cast<int64_t>(maxInt) * 2));
    BOOST_CHECK(2 * feeRate == CFeeRate(static_cast<int64_t>(maxInt) * 2));
    // Check that CFeeRate multiplication saturates instead of overflowing.
    const CAmount max_amount{std::numeric_limits<CAmount>::max()};
    const CAmount min_amount{std::numeric_limits<CAmount>::min()};
    BOOST_CHECK(CFeeRate(max_amount) * 2 == CFeeRate(max_amount));
    BOOST_CHECK(2 * CFeeRate(max_amount) == CFeeRate(max_amount));
    BOOST_CHECK(CFeeRate(min_amount) * 2 == CFeeRate(min_amount));
    BOOST_CHECK(2 * CFeeRate(min_amount) == CFeeRate(min_amount));
    BOOST_CHECK(CFeeRate(min_amount) * -1 == CFeeRate(max_amount));
    BOOST_CHECK(-1 * CFeeRate(min_amount) == CFeeRate(max_amount));
    BOOST_CHECK(CFeeRate(max_amount) * -2 == CFeeRate(min_amount));
    BOOST_CHECK(-2 * CFeeRate(max_amount) == CFeeRate(min_amount));
    // check with zero fee rate
    feeRate = CFeeRate(0);
    BOOST_CHECK(feeRate * 5 == CFeeRate(0));
    BOOST_CHECK(5 * feeRate == CFeeRate(0));
}

BOOST_AUTO_TEST_CASE(BinaryOperatorTest)
{
    CFeeRate a, b;
    a = CFeeRate(1);
    b = CFeeRate(2);
    BOOST_CHECK(a < b);
    BOOST_CHECK(b > a);
    BOOST_CHECK(a == a);
    BOOST_CHECK(a <= b);
    BOOST_CHECK(a <= a);
    BOOST_CHECK(b >= a);
    BOOST_CHECK(b >= b);
    // a should be 0.00000002 BTC/kvB now
    a += a;
    BOOST_CHECK(a == b);

    const CFeeRate default_zero;
    BOOST_CHECK(default_zero == CFeeRate(0));
    BOOST_CHECK(default_zero < CFeeRate(1));
    BOOST_CHECK(default_zero > CFeeRate(-1));
    BOOST_CHECK(CFeeRate(1) > default_zero);
    BOOST_CHECK(CFeeRate(-1) < default_zero);
    BOOST_CHECK(default_zero != CFeeRate(1));
    BOOST_CHECK(default_zero != CFeeRate(-1));
}

BOOST_AUTO_TEST_CASE(FeeRateAdditionZeroIdentity)
{
    const CFeeRate fractional_rate{CAmount(1), 1001};
    BOOST_CHECK(fractional_rate > CFeeRate(0));
    BOOST_CHECK(fractional_rate < CFeeRate(1));

    CFeeRate add_default_zero{fractional_rate};
    add_default_zero += CFeeRate{};
    BOOST_CHECK(add_default_zero == fractional_rate);

    CFeeRate add_integer_zero{fractional_rate};
    add_integer_zero += CFeeRate{0};
    BOOST_CHECK(add_integer_zero == fractional_rate);

    CFeeRate default_zero;
    default_zero += fractional_rate;
    BOOST_CHECK(default_zero == fractional_rate);

    CFeeRate integer_zero{0};
    integer_zero += fractional_rate;
    BOOST_CHECK(integer_zero == fractional_rate);
}

BOOST_AUTO_TEST_CASE(FeeRateAdditionPreservesExactRates)
{
    const CFeeRate fractional_rate{CAmount(1), 1001};

    CFeeRate doubled_rate{fractional_rate};
    doubled_rate += fractional_rate;
    BOOST_CHECK(doubled_rate == CFeeRate(CAmount(2), 1001));
    BOOST_CHECK(doubled_rate > fractional_rate);
    BOOST_CHECK_EQUAL(doubled_rate.GetFee(1001), CAmount(2));
    BOOST_CHECK_EQUAL(doubled_rate.GetFeePerK(), CAmount(1));

    CFeeRate bumped_rate{fractional_rate};
    bumped_rate += CFeeRate{1};
    BOOST_CHECK(bumped_rate == CFeeRate(CAmount(2001), 1001000));
    BOOST_CHECK(bumped_rate > CFeeRate{1});
    BOOST_CHECK_EQUAL(bumped_rate.GetFee(1001000), CAmount(2001));
}

BOOST_AUTO_TEST_CASE(ToStringTest)
{
    CFeeRate feeRate;
    feeRate = CFeeRate(1);
    BOOST_CHECK_EQUAL(feeRate.ToString(), "0.00000001 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::BTC_KVB), "0.00000001 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::SAT_VB), "0.001 sat/vB");

    feeRate = CFeeRate(-1);
    BOOST_CHECK_EQUAL(feeRate.ToString(), "-0.00000001 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::BTC_KVB), "-0.00000001 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::SAT_VB), "-0.001 sat/vB");

    feeRate = CFeeRate(-1000);
    BOOST_CHECK_EQUAL(feeRate.ToString(), "-0.00001000 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::BTC_KVB), "-0.00001000 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::SAT_VB), "-1.000 sat/vB");

    feeRate = CFeeRate(std::numeric_limits<CAmount>::min());
    BOOST_CHECK_EQUAL(feeRate.ToString(), "-92233720368.54775808 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::BTC_KVB), "-92233720368.54775808 BTC/kvB");
    BOOST_CHECK_EQUAL(feeRate.ToString(FeeRateFormat::SAT_VB), "-9223372036854775.808 sat/vB");
}

BOOST_AUTO_TEST_SUITE_END()
