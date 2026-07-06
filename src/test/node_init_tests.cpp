// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <interfaces/init.h>
#include <logging.h>
#include <rpc/server.h>

#include <boost/test/unit_test.hpp>
#include <test/util/common.h>
#include <test/util/setup_common.h>

#include <cstdint>

using node::NodeContext;

//! Like BasicTestingSetup, but using regtest network instead of mainnet.
struct InitTestSetup : BasicTestingSetup {
    InitTestSetup() : BasicTestingSetup{ChainType::REGTEST} {}
};

BOOST_FIXTURE_TEST_SUITE(node_init_tests, InitTestSetup)

//! Custom implementation of interfaces::Init for testing.
class TestInit : public interfaces::Init
{
public:
    TestInit(NodeContext& node) : m_node(node)
    {
        InitContext(m_node);
        m_node.init = this;
    }
    std::unique_ptr<interfaces::Chain> makeChain() override { return interfaces::MakeChain(m_node); }
    std::unique_ptr<interfaces::WalletLoader> makeWalletLoader(interfaces::Chain& chain) override
    {
        return MakeWalletLoader(chain, *Assert(m_node.args));
    }
    NodeContext& m_node;
};

BOOST_AUTO_TEST_CASE(block_storage_space_warning_units)
{
    constexpr uint64_t GB{1'000'000'000};
    constexpr uint64_t MIB{1024 * 1024};

    BOOST_CHECK_EQUAL(node::CalculateBlockStorageSpaceRequired(856, std::nullopt), 856 * GB);
    BOOST_CHECK_EQUAL(node::CalculateBlockStorageSpaceWarningGB(856 * GB), 856);

    const uint64_t prune_target{550 * MIB};
    BOOST_CHECK_EQUAL(node::CalculateBlockStorageSpaceRequired(856, prune_target), prune_target);
    BOOST_CHECK_EQUAL(node::CalculateBlockStorageSpaceWarningGB(prune_target), 1);

    BOOST_CHECK_EQUAL(node::CalculateBlockStorageSpaceRequired(1, 2 * GB), GB);
    BOOST_CHECK_EQUAL(node::CalculateBlockStorageSpaceWarningGB(GB + 1), 2);
}

BOOST_AUTO_TEST_CASE(init_test)
{
    // BasicTestingSetup parses command-line arguments but does not read config
    // files. AppInitMain's logging expects both config paths to be initialized.
    LogInstance().DisconnectTestLogger();
    std::string error;
    BOOST_REQUIRE_MESSAGE(m_node.args->ReadConfigFiles(error, /*ignore_invalid_keys=*/true), error);

    // Prevent the test from trying to listen on ports 8332 and 8333.
    m_node.args->ForceSetArg("-server", "0");
    m_node.args->ForceSetArg("-listen", "0");

    // Run through initialization and shutdown code.
    TestInit init{m_node};
    BOOST_CHECK(AppInitInterfaces(m_node));
    BOOST_CHECK(AppInitMain(m_node));
    Interrupt(m_node);
    Shutdown(m_node);
}

BOOST_AUTO_TEST_SUITE_END()
