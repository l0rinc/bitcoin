// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
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

BOOST_AUTO_TEST_SUITE_END()
