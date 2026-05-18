// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <consensus/params.h>
#include <kernel/chainparams.h>
#include <test/util/setup_common.h>

BOOST_FIXTURE_TEST_SUITE(chainparams_tests, BasicTestingSetup)

//! Document the regtest defaults for buried deployments and DEPLOYMENT_TESTDUMMY,
//! and confirm that DeploymentOptions overrides them via ApplyDeploymentOptions.
BOOST_AUTO_TEST_CASE(regtest_deployment_options)
{
    // Empty options: assert current regtest defaults.
    {
        const auto params{CChainParams::RegTest({})};
        const auto& c{params->GetConsensus()};
        BOOST_CHECK_EQUAL(c.BIP34Height, 1);
        BOOST_CHECK_EQUAL(c.BIP65Height, 1);
        BOOST_CHECK_EQUAL(c.BIP66Height, 1);
        BOOST_CHECK_EQUAL(c.CSVHeight, 1);
        BOOST_CHECK_EQUAL(c.SegwitHeight, 0);
        const auto& testdummy{c.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY]};
        BOOST_CHECK_EQUAL(testdummy.nStartTime, 0);
        BOOST_CHECK_EQUAL(testdummy.nTimeout, Consensus::BIP9Deployment::NO_TIMEOUT);
        BOOST_CHECK_EQUAL(testdummy.min_activation_height, 0);
        BOOST_CHECK_EQUAL(testdummy.threshold, 108u);
        BOOST_CHECK_EQUAL(testdummy.period, 144);
    }

    // Custom activation heights override the defaults.
    {
        CChainParams::RegTestOptions opts;
        opts.dep_opts.activation_heights[Consensus::DEPLOYMENT_HEIGHTINCB] = 111;
        opts.dep_opts.activation_heights[Consensus::DEPLOYMENT_CLTV] = 222;
        opts.dep_opts.activation_heights[Consensus::DEPLOYMENT_DERSIG] = 333;
        opts.dep_opts.activation_heights[Consensus::DEPLOYMENT_CSV] = 444;
        opts.dep_opts.activation_heights[Consensus::DEPLOYMENT_SEGWIT] = 555;
        const auto params{CChainParams::RegTest(opts)};
        const auto& c{params->GetConsensus()};
        BOOST_CHECK_EQUAL(c.BIP34Height, 111);
        BOOST_CHECK_EQUAL(c.BIP65Height, 222);
        BOOST_CHECK_EQUAL(c.BIP66Height, 333);
        BOOST_CHECK_EQUAL(c.CSVHeight, 444);
        BOOST_CHECK_EQUAL(c.SegwitHeight, 555);
    }

    // Custom version-bits parameters override the defaults for DEPLOYMENT_TESTDUMMY.
    {
        CChainParams::RegTestOptions opts;
        opts.dep_opts.version_bits_parameters[Consensus::DEPLOYMENT_TESTDUMMY] = {
            .start_time = 1000,
            .timeout = 2000,
            .min_activation_height = 500,
        };
        const auto params{CChainParams::RegTest(opts)};
        const auto& testdummy{params->GetConsensus().vDeployments[Consensus::DEPLOYMENT_TESTDUMMY]};
        BOOST_CHECK_EQUAL(testdummy.nStartTime, 1000);
        BOOST_CHECK_EQUAL(testdummy.nTimeout, 2000);
        BOOST_CHECK_EQUAL(testdummy.min_activation_height, 500);
    }
}

BOOST_AUTO_TEST_SUITE_END()
