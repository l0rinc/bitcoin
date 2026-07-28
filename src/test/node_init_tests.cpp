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

#include <limits>
#include <string>

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

BOOST_AUTO_TEST_CASE(init_test)
{
    // Clear state set by BasicTestingSetup that AppInitMain assumes is unset.
    LogInstance().DisconnectTestLogger();
    m_node.args->SetConfigFilePath({});

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

BOOST_AUTO_TEST_CASE(init_rejects_out_of_range_buffer_arguments)
{
    LogInstance().DisconnectTestLogger();
    m_node.args->ForceSetArg("-server", "0");
    m_node.args->ForceSetArg("-listen", "0");
    m_node.args->ForceSetArg("-maxsendbuffer", std::to_string(std::numeric_limits<unsigned int>::max() / 1000 + 1));

    BOOST_CHECK(!AppInitParameterInteraction(*m_node.args));

    m_node.args->ForceSetArg("-maxsendbuffer", "1000");
    m_node.args->ForceSetArg("-maxreceivebuffer", std::to_string(std::numeric_limits<unsigned int>::max() / 1000 + 1));
    BOOST_CHECK(!AppInitParameterInteraction(*m_node.args));
}

BOOST_AUTO_TEST_CASE(init_rejects_overflowing_prune_argument)
{
    LogInstance().DisconnectTestLogger();
    m_node.args->ForceSetArg("-server", "0");
    m_node.args->ForceSetArg("-listen", "0");
    m_node.args->ForceSetArg("-listenonion", "0");
    m_node.args->ForceSetArg("-prune", "17592186044416");

    BOOST_CHECK(!AppInitParameterInteraction(*m_node.args));
}

BOOST_AUTO_TEST_SUITE_END()
