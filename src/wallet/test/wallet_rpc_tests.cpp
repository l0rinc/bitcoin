// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/request.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <wallet/rpc/util.h>

#include <boost/test/unit_test.hpp>

#include <optional>
#include <string>

namespace wallet {
static std::string TestWalletName(const std::string& endpoint, std::optional<std::string> parameter = std::nullopt)
{
    JSONRPCRequest req;
    req.URI = endpoint;
    return EnsureUniqueWalletName(req, parameter);
}

BOOST_FIXTURE_TEST_SUITE(wallet_rpc_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ensure_unique_wallet_name)
{
    // EnsureUniqueWalletName should only return if exactly one unique wallet name is provided
    CHECK_EQUAL(TestWalletName("/wallet/foo"), std::string_view{"foo"});
    CHECK_EQUAL(TestWalletName("/wallet/foo", "foo"), std::string_view{"foo"});
    CHECK_EQUAL(TestWalletName("/", "foo"), std::string_view{"foo"});
    CHECK_EQUAL(TestWalletName("/bar", "foo"), std::string_view{"foo"});

    CHECK_THROW(TestWalletName("/"), UniValue);
    CHECK_THROW(TestWalletName("/foo"), UniValue);
    CHECK_THROW(TestWalletName("/wallet/foo", "bar"), UniValue);
    CHECK_THROW(TestWalletName("/wallet/foo", "foobar"), UniValue);
    CHECK_THROW(TestWalletName("/wallet/foobar", "foo"), UniValue);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
