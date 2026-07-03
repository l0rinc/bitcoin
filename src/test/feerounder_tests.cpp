// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/fees/block_policy_estimator.h>

#include <boost/test/unit_test.hpp>

#include <set>

BOOST_AUTO_TEST_SUITE(feerounder_tests)

BOOST_AUTO_TEST_CASE(FeeRounder)
{
    static constexpr CAmount MAX_FILTER_FEERATE{10'000'000};
    FastRandomContext rng{/*fDeterministic=*/true};
    FeeFilterRounder fee_rounder{CFeeRate{1000}, rng};

    // check that 1000 rounds to 974 or 1071
    std::set<CAmount> results;
    while (results.size() < 2) {
        results.emplace(fee_rounder.round(1000));
    }
    BOOST_CHECK_EQUAL(*results.begin(), 974);
    BOOST_CHECK_EQUAL(*++results.begin(), 1071);

    // check that negative amounts rounds to 0
    BOOST_CHECK_EQUAL(fee_rounder.round(-0), 0);
    BOOST_CHECK_EQUAL(fee_rounder.round(-1), 0);

    // check that MAX_MONEY rounds to 9170997
    BOOST_CHECK_EQUAL(fee_rounder.round(MAX_MONEY), 9170997);

    FastRandomContext high_rng{/*fDeterministic=*/true};
    FeeFilterRounder high_fee_rounder{CFeeRate{MAX_FILTER_FEERATE * 5}, high_rng};
    BOOST_CHECK_LE(high_fee_rounder.round(MAX_MONEY), MAX_FILTER_FEERATE);
    BOOST_CHECK_EQUAL(high_fee_rounder.round(-MAX_MONEY), 0);
}

BOOST_AUTO_TEST_SUITE_END()
