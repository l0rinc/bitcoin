// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <string>

BOOST_FIXTURE_TEST_SUITE(chainparams_tests, BasicTestingSetup)

static bool HasDnsSeed(const CChainParams& params, const std::string& seed)
{
    const auto& seeds{params.DNSSeeds()};
    return std::find(seeds.begin(), seeds.end(), seed) != seeds.end();
}

BOOST_AUTO_TEST_CASE(dns_seed_removals)
{
    const auto main_params{CreateChainParams(*m_node.args, ChainType::MAIN)};
    BOOST_CHECK(!HasDnsSeed(*main_params, "seed.btc.petertodd.net."));

    const auto testnet_params{CreateChainParams(*m_node.args, ChainType::TESTNET)};
    BOOST_CHECK(!HasDnsSeed(*testnet_params, "seed.tbtc.petertodd.net."));
}

BOOST_AUTO_TEST_CASE(signet_block_time)
{
    const auto default_signet_params{CreateChainParams(ArgsManager{}, ChainType::SIGNET)};
    BOOST_CHECK_EQUAL(default_signet_params->GetConsensus().nPowTargetSpacing, 10 * 60);

    ArgsManager custom_signet_args;
    custom_signet_args.ForceSetArg("-signetchallenge", "51");
    custom_signet_args.ForceSetArg("-signetblocktime", "30");
    const auto custom_signet_params{CreateChainParams(custom_signet_args, ChainType::SIGNET)};
    BOOST_CHECK_EQUAL(custom_signet_params->GetConsensus().nPowTargetSpacing, 30);

    ArgsManager missing_challenge_args;
    missing_challenge_args.ForceSetArg("-signetblocktime", "30");
    BOOST_CHECK_THROW(CreateChainParams(missing_challenge_args, ChainType::SIGNET), std::runtime_error);

    ArgsManager zero_block_time_args;
    zero_block_time_args.ForceSetArg("-signetchallenge", "51");
    zero_block_time_args.ForceSetArg("-signetblocktime", "0");
    BOOST_CHECK_THROW(CreateChainParams(zero_block_time_args, ChainType::SIGNET), std::runtime_error);

    ArgsManager negative_block_time_args;
    negative_block_time_args.ForceSetArg("-signetchallenge", "51");
    negative_block_time_args.ForceSetArg("-signetblocktime", "-1");
    BOOST_CHECK_THROW(CreateChainParams(negative_block_time_args, ChainType::SIGNET), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
