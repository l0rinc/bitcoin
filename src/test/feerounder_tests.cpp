// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/fees/block_policy_estimator.h>
#include <test/util/check.h>

#include <boost/test/unit_test.hpp>

#include <set>

BOOST_AUTO_TEST_SUITE(fee_rounder_tests)

BOOST_AUTO_TEST_CASE(FeeRounder)
{
    FastRandomContext rng{/*fDeterministic=*/true};
    FeeFilterRounder fee_rounder{CFeeRate{1000}, rng};

    // check that 1000 rounds to 974 or 1071
    std::set<CAmount> results;
    while (results.size() < 2) {
        results.emplace(fee_rounder.round(1000));
    }
    CHECK_EQUAL(*results.begin(), std::remove_cvref_t<decltype(*results.begin())>{974});
    CHECK_EQUAL(*++results.begin(), std::remove_cvref_t<decltype(*++results.begin())>{1071});

    // check that negative amounts rounds to 0
    CHECK_EQUAL(fee_rounder.round(-0), std::remove_cvref_t<decltype(fee_rounder.round(-0))>{0});
    CHECK_EQUAL(fee_rounder.round(-1), std::remove_cvref_t<decltype(fee_rounder.round(-1))>{0});

    // check that MAX_MONEY rounds to 9170997
    CHECK_EQUAL(fee_rounder.round(MAX_MONEY), std::remove_cvref_t<decltype(fee_rounder.round(MAX_MONEY))>{9170997});
}

BOOST_AUTO_TEST_SUITE_END()
