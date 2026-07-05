// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

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

BOOST_AUTO_TEST_CASE(network_security_parameters)
{
    struct ExpectedSecurityParams {
        ChainType chain;
        std::string min_chainwork;
        std::string assume_valid;
    };

    const std::array expected_params{
        ExpectedSecurityParams{
            ChainType::MAIN,
            "0000000000000000000000000000000000000001128750f82f4c366153a3a030",
            "00000000000000000000ccebd6d74d9194d8dcdc1d177c478e094bfad51ba5ac",
        },
        ExpectedSecurityParams{
            ChainType::TESTNET,
            "0000000000000000000000000000000000000000000017dde1c649f3708d14b6",
            "000000007a61e4230b28ac5cb6b5e5a0130de37ac1faf2f8987d2fa6505b67f4",
        },
        ExpectedSecurityParams{
            ChainType::TESTNET4,
            "0000000000000000000000000000000000000000000009a0fe15d0177d086304",
            "0000000002368b1e4ee27e2e85676ae6f9f9e69579b29093e9a82c170bf7cf8a",
        },
        ExpectedSecurityParams{
            ChainType::SIGNET,
            "00000000000000000000000000000000000000000000000000000b463ea0a4b8",
            "00000008414aab61092ef93f1aacc54cf9e9f16af29ddad493b908a01ff5c329",
        },
    };

    for (const auto& expected : expected_params) {
        const auto params{CreateChainParams(*m_node.args, expected.chain)};
        BOOST_CHECK_EQUAL(params->GetConsensus().nMinimumChainWork.ToString(), expected.min_chainwork);
        BOOST_CHECK_EQUAL(params->GetConsensus().defaultAssumeValid.ToString(), expected.assume_valid);
    }
}

BOOST_AUTO_TEST_CASE(testnet4_assumeutxo_parameters)
{
    const auto params{CreateChainParams(*m_node.args, ChainType::TESTNET4)};

    const std::vector expected_heights{90'000, 120'000};
    const auto actual_heights{params->GetAvailableSnapshotHeights()};
    BOOST_CHECK_EQUAL_COLLECTIONS(actual_heights.begin(), actual_heights.end(), expected_heights.begin(),
                                  expected_heights.end());

    const auto data_90000{*params->AssumeutxoForHeight(90'000)};
    BOOST_CHECK_EQUAL(data_90000.hash_serialized.ToString(),
                      "784fb5e98241de66fdd429f4392155c9e7db5c017148e66e8fdbc95746f8b9b5");
    BOOST_CHECK_EQUAL(data_90000.m_chain_tx_count, 11347043U);
    BOOST_CHECK_EQUAL(data_90000.blockhash.ToString(),
                      "0000000002ebe8bcda020e0dd6ccfbdfac531d2f6a81457191b99fc2df2dbe3b");

    const auto data_120000{*params->AssumeutxoForHeight(120'000)};
    BOOST_CHECK_EQUAL(data_120000.hash_serialized.ToString(),
                      "10b05d05ad468d0971162e1b222a4aa66caca89da2bb2a93f8f37fb29c4794b0");
    BOOST_CHECK_EQUAL(data_120000.m_chain_tx_count, 14141057U);
    BOOST_CHECK_EQUAL(data_120000.blockhash.ToString(),
                      "000000000bd2317e51b3c5794981c35ba894ce27d3e772d5c39ecd9cbce01dc8");

    const auto data_by_hash{
        *params->AssumeutxoForBlockhash(uint256{"0000000002ebe8bcda020e0dd6ccfbdfac531d2f6a81457191b99fc2df2dbe3b"})};
    BOOST_CHECK_EQUAL(data_by_hash.height, 90'000);
    BOOST_CHECK_EQUAL(data_by_hash.hash_serialized.ToString(), data_90000.hash_serialized.ToString());
}

BOOST_AUTO_TEST_SUITE_END()
