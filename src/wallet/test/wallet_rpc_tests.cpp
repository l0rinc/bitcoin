// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/request.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <wallet/context.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
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

static std::string TestWalletNamePointer(const std::string& endpoint, std::optional<std::string> parameter = std::nullopt)
{
    JSONRPCRequest req;
    req.URI = endpoint;
    return EnsureUniqueWalletName(req, parameter ? &*parameter : nullptr);
}

BOOST_FIXTURE_TEST_SUITE(wallet_rpc_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(ensure_unique_wallet_name)
{
    // EnsureUniqueWalletName should only return if exactly one unique wallet name is provided
    BOOST_CHECK_EQUAL(TestWalletName("/wallet/foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/wallet/foo", "foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/", "foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/bar", "foo"), "foo");

    BOOST_CHECK_THROW(TestWalletName("/"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/foo"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foo", "bar"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foo", "foobar"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foobar", "foo"), UniValue);

    BOOST_CHECK_EQUAL(TestWalletNamePointer("/wallet/foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletNamePointer("/", "foo"), "foo");
    BOOST_CHECK_THROW(TestWalletNamePointer("/wallet/foo", "bar"), UniValue);
}

BOOST_AUTO_TEST_CASE(wallet_restriction)
{
    WalletContext context;
    auto wallet_a{std::make_shared<CWallet>(/*chain=*/nullptr, "wallet_a", CreateMockableWalletDatabase())};
    auto wallet_b{std::make_shared<CWallet>(/*chain=*/nullptr, "wallet_b", CreateMockableWalletDatabase())};
    BOOST_REQUIRE(AddWallet(context, wallet_a));
    BOOST_REQUIRE(AddWallet(context, wallet_b));

    JSONRPCRequest request;
    request.context = &context;

    request.m_wallet_restriction = "";
    request.URI = "/wallet/wallet_b";
    BOOST_CHECK_EQUAL(GetWalletForJSONRPCRequest(request), wallet_b);
    request.URI = "/";
    BOOST_CHECK_THROW(GetWalletForJSONRPCRequest(request), UniValue);

    request.m_wallet_restriction = "wallet_a";
    BOOST_CHECK_EQUAL(GetWalletForJSONRPCRequest(request), wallet_a);
    request.URI = "/wallet/wallet_a";
    BOOST_CHECK_EQUAL(GetWalletForJSONRPCRequest(request), wallet_a);
    request.URI = "/wallet/wallet_b";
    BOOST_CHECK_THROW(GetWalletForJSONRPCRequest(request), UniValue);

    request.m_wallet_restriction = "-";
    request.URI = "/";
    BOOST_CHECK_THROW(GetWalletForJSONRPCRequest(request), UniValue);
    request.URI = "/wallet/wallet_a";
    BOOST_CHECK_THROW(GetWalletForJSONRPCRequest(request), UniValue);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
