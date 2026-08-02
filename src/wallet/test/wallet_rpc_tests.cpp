// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <key_io.h>
#include <merkleblock.h>
#include <node/chainstate.h>
#include <rpc/protocol.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <univalue.h>
#include <wallet/context.h>
#include <wallet/rpc/wallet.h>
#include <wallet/rpc/util.h>
#include <wallet/sqlite.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace wallet {

class FailingWriteSQLiteBatch final : public MockableSQLiteBatch
{
    bool& m_fail_writes;

public:
    FailingWriteSQLiteBatch(SQLiteDatabase& database, bool& fail_writes)
        : MockableSQLiteBatch(database), m_fail_writes(fail_writes) {}

protected:
    bool WriteKey(DataStream&& key, DataStream&& value, bool overwrite) override
    {
        if (m_fail_writes) return false;
        return SQLiteBatch::WriteKey(std::move(key), std::move(value), overwrite);
    }

    bool EraseKey(DataStream&& key) override
    {
        if (m_fail_writes) return false;
        return SQLiteBatch::EraseKey(std::move(key));
    }
};

class FailingWriteSQLiteDatabase final : public MockableSQLiteDatabase
{
    bool m_fail_writes{false};

public:
    void SetFailWrites(bool fail_writes) { m_fail_writes = fail_writes; }

    std::unique_ptr<DatabaseBatch> MakeBatch() override
    {
        return std::make_unique<FailingWriteSQLiteBatch>(*this, m_fail_writes);
    }
};

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
    BOOST_CHECK_EQUAL(TestWalletName("/wallet/foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/wallet/foo", "foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/", "foo"), "foo");
    BOOST_CHECK_EQUAL(TestWalletName("/bar", "foo"), "foo");

    BOOST_CHECK_THROW(TestWalletName("/"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/foo"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foo", "bar"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foo", "foobar"), UniValue);
    BOOST_CHECK_THROW(TestWalletName("/wallet/foobar", "foo"), UniValue);
}

BOOST_FIXTURE_TEST_CASE(importprunedfunds_reports_wallet_write_failure, TestChain100Setup)
{
    const CScript coinbase_script{GetScriptForRawPubKey(coinbaseKey.GetPubKey())};
    const CBlock block{CreateAndProcessBlock({}, coinbase_script)};
    const CTransactionRef transaction{block.vtx.front()};

    auto database{std::make_unique<FailingWriteSQLiteDatabase>()};
    auto* database_ptr{database.get()};
    auto wallet{std::make_shared<CWallet>(m_node.chain.get(), "", std::move(database))};
    WalletContext context;
    context.chain = m_node.chain.get();

    int tip_height;
    uint256 tip_hash;
    {
        LOCK(::cs_main);
        const auto* tip{Assert(m_node.chainman)->ActiveChain().Tip()};
        tip_height = tip->nHeight;
        tip_hash = tip->GetBlockHash();
    }
    {
        LOCK(wallet->cs_wallet);
        BOOST_REQUIRE(CreateDescriptor(*wallet, "combo(" + EncodeSecret(coinbaseKey) + ")", true));
        wallet->SetLastBlockProcessed(tip_height, tip_hash);
    }
    BOOST_REQUIRE(AddWallet(context, wallet));

    const CMerkleBlock merkle_block{block, std::set<Txid>{transaction->GetHash()}};
    DataStream proof_stream;
    proof_stream << merkle_block;

    const CRPCCommand* command{nullptr};
    for (const auto& candidate : GetWalletRPCCommands()) {
        if (candidate.name == "importprunedfunds") {
            command = &candidate;
            break;
        }
    }
    BOOST_REQUIRE(command);

    JSONRPCRequest request;
    request.context = &context;
    request.params = UniValue{UniValue::VARR};
    request.params.push_back(EncodeHexTx(*transaction));
    request.params.push_back(HexStr(proof_stream));

    database_ptr->SetFailWrites(true);
    UniValue error;
    try {
        UniValue result;
        command->actor(request, result, true);
    } catch (const UniValue& caught_error) {
        error = caught_error;
    }

    BOOST_REQUIRE(error.isObject());
    BOOST_CHECK_EQUAL(error["code"].getInt<int>(), RPC_WALLET_ERROR);
    BOOST_CHECK_EQUAL(error["message"].get_str(), "Wallet db error, transaction import failed");
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
