// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>
#include <chain.h>
#include <consensus/amount.h>
#include <consensus/validation.h>
#include <kernel/mempool_entry.h>
#include <kernel/mempool_removal_reason.h>
#include <kernel/types.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <scheduler.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <test/util/txmempool.h>
#include <util/check.h>
#include <validationinterface.h>

#include <atomic>
#include <memory>

BOOST_FIXTURE_TEST_SUITE(validationinterface_tests, ChainTestingSetup)

struct TestSubscriberNoop final : public CValidationInterface {
    void BlockChecked(const std::shared_ptr<const CBlock>&, const BlockValidationState&) override {}
};

struct BlockNotificationSubscriber final : public CValidationInterface {
    int m_connected{0};
    int m_disconnected{0};
    int m_new_pow_valid{0};

    void BlockConnected(const kernel::ChainstateRole&, const std::shared_ptr<const CBlock>&, const CBlockIndex*) override
    {
        ++m_connected;
    }

    void BlockDisconnected(const std::shared_ptr<const CBlock>&, const CBlockIndex*) override
    {
        ++m_disconnected;
    }

    void NewPoWValidBlock(const CBlockIndex*, const std::shared_ptr<const CBlock>&) override
    {
        ++m_new_pow_valid;
    }
};

struct MempoolPayloadSubscriber final : public CValidationInterface {
    const Txid m_txid;
    const CAmount m_fee;
    const int64_t m_vsize;
    const unsigned int m_height;
    int m_added{0};
    int m_removed{0};
    int m_removed_for_block{0};

    MempoolPayloadSubscriber(Txid txid, CAmount fee, int64_t vsize, unsigned int height)
        : m_txid{txid}, m_fee{fee}, m_vsize{vsize}, m_height{height}
    {
    }

    void CheckInfo(const TransactionInfo& info)
    {
        BOOST_REQUIRE(info.m_tx);
        BOOST_CHECK(info.m_tx->GetHash() == m_txid);
        BOOST_CHECK(MoneyRange(info.m_fee));
        BOOST_CHECK_EQUAL(info.m_fee, m_fee);
        BOOST_CHECK_EQUAL(info.m_virtual_transaction_size, m_vsize);
        BOOST_CHECK_EQUAL(info.txHeight, m_height);
    }

    void TransactionAddedToMempool(const NewMempoolTransactionInfo& tx, uint64_t mempool_sequence) override
    {
        CheckInfo(tx.info);
        BOOST_CHECK_GT(mempool_sequence, 0U);
        ++m_added;
    }

    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override
    {
        BOOST_REQUIRE(tx);
        BOOST_CHECK(tx->GetHash() == m_txid);
        BOOST_CHECK(reason == MemPoolRemovalReason::EXPIRY);
        BOOST_CHECK_GT(mempool_sequence, 0U);
        ++m_removed;
    }

    void MempoolTransactionsRemovedForBlock(const std::vector<RemovedMempoolTransactionInfo>& txs_removed_for_block, unsigned int nBlockHeight) override
    {
        BOOST_CHECK_EQUAL(nBlockHeight, m_height + 1);
        BOOST_REQUIRE_EQUAL(txs_removed_for_block.size(), 1);
        CheckInfo(txs_removed_for_block.front().info);
        ++m_removed_for_block;
    }
};

BOOST_AUTO_TEST_CASE(mempool_signal_payload_contracts)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vout.emplace_back(COIN, CScript{});
    const CTransactionRef tx{MakeTransactionRef(mtx)};
    const CAmount fee{1000};
    const unsigned int height{7};
    const CTxMemPoolEntry entry{TestMemPoolEntryHelper{}.Fee(fee).Height(height).FromTx(tx)};
    const int64_t vsize{entry.GetTxSize()};

    auto sub{std::make_shared<MempoolPayloadSubscriber>(tx->GetHash(), fee, vsize, height)};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);

    const NewMempoolTransactionInfo new_tx{tx, fee, vsize, height,
                                           /*mempool_limit_bypassed=*/false,
                                           /*submitted_in_package=*/false,
                                           /*chainstate_is_current=*/true,
                                           /*has_no_mempool_parents=*/true};
    m_node.validation_signals->TransactionAddedToMempool(new_tx, /*mempool_sequence=*/1);
    m_node.validation_signals->TransactionRemovedFromMempool(tx, MemPoolRemovalReason::EXPIRY, /*mempool_sequence=*/2);

    m_node.validation_signals->MempoolTransactionsRemovedForBlock({RemovedMempoolTransactionInfo{entry}}, height + 1);
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(sub);

    BOOST_CHECK_EQUAL(sub->m_added, 1);
    BOOST_CHECK_EQUAL(sub->m_removed, 1);
    BOOST_CHECK_EQUAL(sub->m_removed_for_block, 1);
}

BOOST_AUTO_TEST_CASE(block_signal_rejects_null_tx_refs)
{
    test_only_CheckFailuresAreExceptionsNotAborts failed_asserts_throw{};

    CBlock parent_block;
    const uint256 parent_hash{parent_block.GetHash()};
    CBlockIndex parent_index;
    parent_index.phashBlock = &parent_hash;
    parent_index.nHeight = 0;

    CBlock resized_block;
    resized_block.hashPrevBlock = parent_hash;
    resized_block.vtx.resize(1);

    const uint256 resized_block_hash{resized_block.GetHash()};
    CBlockIndex resized_block_index;
    resized_block_index.phashBlock = &resized_block_hash;
    resized_block_index.pprev = &parent_index;
    resized_block_index.nHeight = 1;

    auto sub{std::make_shared<BlockNotificationSubscriber>()};
    m_node.validation_signals->RegisterSharedValidationInterface(sub);

    BOOST_CHECK_THROW(m_node.validation_signals->BlockConnected(kernel::ChainstateRole{}, std::make_shared<CBlock>(resized_block), &resized_block_index),
                      NonFatalCheckError);
    BOOST_CHECK_THROW(m_node.validation_signals->BlockDisconnected(std::make_shared<CBlock>(resized_block), &resized_block_index),
                      NonFatalCheckError);
    BOOST_CHECK_THROW(m_node.validation_signals->NewPoWValidBlock(&resized_block_index, std::make_shared<CBlock>(resized_block)),
                      NonFatalCheckError);

    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    m_node.validation_signals->UnregisterSharedValidationInterface(sub);

    BOOST_CHECK_EQUAL(sub->m_connected, 0);
    BOOST_CHECK_EQUAL(sub->m_disconnected, 0);
    BOOST_CHECK_EQUAL(sub->m_new_pow_valid, 0);
}

BOOST_AUTO_TEST_CASE(unregister_validation_interface_race)
{
    std::atomic<bool> generate{true};

    // Start thread to generate notifications
    std::thread gen{[&] {
        BlockValidationState state_dummy;
        while (generate) {
            m_node.validation_signals->BlockChecked(std::make_shared<const CBlock>(), state_dummy);
        }
    }};

    // Start thread to consume notifications
    std::thread sub{[&] {
        // keep going for about 1 sec, which is 250k iterations
        for (int i = 0; i < 250000; i++) {
            auto sub = std::make_shared<TestSubscriberNoop>();
            m_node.validation_signals->RegisterSharedValidationInterface(sub);
            m_node.validation_signals->UnregisterSharedValidationInterface(sub);
        }
        // tell the other thread we are done
        generate = false;
    }};

    gen.join();
    sub.join();
    BOOST_CHECK(!generate);
}

class TestInterface : public CValidationInterface
{
public:
    TestInterface(ValidationSignals& signals, std::function<void()> on_call = nullptr, std::function<void()> on_destroy = nullptr)
        : m_on_call(std::move(on_call)), m_on_destroy(std::move(on_destroy)), m_signals{signals}
    {
    }
    virtual ~TestInterface()
    {
        if (m_on_destroy) m_on_destroy();
    }
    void BlockChecked(const std::shared_ptr<const CBlock>& block, const BlockValidationState& state) override
    {
        if (m_on_call) m_on_call();
    }
    void Call()
    {
        BlockValidationState state;
        m_signals.BlockChecked(std::make_shared<const CBlock>(), state);
    }
    std::function<void()> m_on_call;
    std::function<void()> m_on_destroy;
    ValidationSignals& m_signals;
};

// Regression test to ensure UnregisterAllValidationInterfaces calls don't
// destroy a validation interface while it is being called. Bug:
// https://github.com/bitcoin/bitcoin/pull/18551
BOOST_AUTO_TEST_CASE(unregister_all_during_call)
{
    bool destroyed = false;
    auto shared{std::make_shared<TestInterface>(
        *m_node.validation_signals,
        [&] {
            // First call should decrements reference count 2 -> 1
            m_node.validation_signals->UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
            // Second call should not decrement reference count 1 -> 0
            m_node.validation_signals->UnregisterAllValidationInterfaces();
            BOOST_CHECK(!destroyed);
        },
        [&] { destroyed = true; })};
    m_node.validation_signals->RegisterSharedValidationInterface(shared);
    BOOST_CHECK(shared.use_count() == 2);
    shared->Call();
    BOOST_CHECK(shared.use_count() == 1);
    BOOST_CHECK(!destroyed);
    shared.reset();
    BOOST_CHECK(destroyed);
}

BOOST_AUTO_TEST_SUITE_END()
