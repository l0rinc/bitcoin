// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <common/system.h>
#include <node/mempool_args.h>
#include <policy/coin_age_priority.h>
#include <policy/policy.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/time.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

static constexpr auto REMOVAL_REASON_DUMMY = MemPoolRemovalReason::REPLACED;

class MemPoolTest final : public CTxMemPool
{
public:
    using CTxMemPool::GetMinFee;
};

BOOST_AUTO_TEST_CASE(MempoolDustDynamicParse)
{
    static constexpr unsigned int max_target{1008};

    auto off{ParseDustDynamicOpt("off", max_target)};
    BOOST_REQUIRE(off);
    BOOST_CHECK_EQUAL(off->first, 0);
    BOOST_CHECK_EQUAL(off->second, DEFAULT_DUST_RELAY_MULTIPLIER);
    auto zero{ParseDustDynamicOpt("0", max_target)};
    BOOST_REQUIRE(zero);
    BOOST_CHECK_EQUAL(zero->first, 0);
    BOOST_CHECK_EQUAL(zero->second, DEFAULT_DUST_RELAY_MULTIPLIER);
    auto target{ParseDustDynamicOpt("target:6", max_target)};
    BOOST_REQUIRE(target);
    BOOST_CHECK_EQUAL(target->first, -6);
    BOOST_CHECK_EQUAL(target->second, DEFAULT_DUST_RELAY_MULTIPLIER);
    auto target_half{ParseDustDynamicOpt("0.5*target:6", max_target)};
    BOOST_REQUIRE(target_half);
    BOOST_CHECK_EQUAL(target_half->first, -6);
    BOOST_CHECK_EQUAL(target_half->second, 500);
    auto mempool{ParseDustDynamicOpt("mempool:250", max_target)};
    BOOST_REQUIRE(mempool);
    BOOST_CHECK_EQUAL(mempool->first, 250);
    BOOST_CHECK_EQUAL(mempool->second, DEFAULT_DUST_RELAY_MULTIPLIER);
    auto mempool_precise{ParseDustDynamicOpt("10.001*mempool:250", max_target)};
    BOOST_REQUIRE(mempool_precise);
    BOOST_CHECK_EQUAL(mempool_precise->first, 250);
    BOOST_CHECK_EQUAL(mempool_precise->second, 10001);

    BOOST_CHECK(!ParseDustDynamicOpt("0*target:6", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("0.0001*target:6", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("x*target:6", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("target:1", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("target:1009", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("target:x", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("mempool:0", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("mempool:x", max_target));
    BOOST_CHECK(!ParseDustDynamicOpt("unknown", max_target));
}

BOOST_AUTO_TEST_CASE(MempoolLookupTest)
{
    auto& pool = static_cast<MemPoolTest&>(*Assert(m_node.mempool));
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << OP_1;
    tx.vout.resize(1);
    tx.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
    tx.vout[0].nValue = 10 * COIN;

    // Not in the mempool, so can't find it by txid or wtxid
    BOOST_CHECK(!pool.get(tx.GetHash()));
    BOOST_CHECK(!pool.get(CTransaction(tx).GetWitnessHash()));

    TryAddToMempool(pool, entry.Fee(1000LL).FromTx(tx));

    // Lookup by Txid
    BOOST_CHECK(pool.get(tx.GetHash()));

    // Lookup by Wtxid
    BOOST_CHECK(pool.get(CTransaction(tx).GetWitnessHash()));
}

BOOST_AUTO_TEST_CASE(MempoolPermitEphemeralParse)
{
    auto parse_options = [](const std::string& arg) {
        ArgsManager argsman;
        argsman.ForceSetArg("-permitephemeral", arg);
        kernel::MemPoolOptions opts;
        const auto result{ApplyArgsManOptions(argsman, Params(), opts)};
        BOOST_REQUIRE(result);
        return opts;
    };

    auto all{parse_options("1")};
    BOOST_CHECK(all.permitephemeral_anchor);
    BOOST_CHECK(all.permitephemeral_send);
    BOOST_CHECK(all.permitephemeral_dust);

    auto none{parse_options("reject")};
    BOOST_CHECK(!none.permitephemeral_anchor);
    BOOST_CHECK(!none.permitephemeral_send);
    BOOST_CHECK(!none.permitephemeral_dust);

    auto anchor_only{parse_options("anchor,-send,-dust")};
    BOOST_CHECK(anchor_only.permitephemeral_anchor);
    BOOST_CHECK(!anchor_only.permitephemeral_send);
    BOOST_CHECK(!anchor_only.permitephemeral_dust);

    auto send_zero{parse_options("send,-dust")};
    BOOST_CHECK(send_zero.permitephemeral_anchor);
    BOOST_CHECK(send_zero.permitephemeral_send);
    BOOST_CHECK(!send_zero.permitephemeral_dust);

    auto send_dust{parse_options("send,dust")};
    BOOST_CHECK(send_dust.permitephemeral_anchor);
    BOOST_CHECK(send_dust.permitephemeral_send);
    BOOST_CHECK(send_dust.permitephemeral_dust);

    auto dust_without_anchor{parse_options("-anchor,dust")};
    BOOST_CHECK(!dust_without_anchor.permitephemeral_anchor);
    BOOST_CHECK(dust_without_anchor.permitephemeral_send);
    BOOST_CHECK(dust_without_anchor.permitephemeral_dust);

    auto unknown{parse_options("unknown")};
    BOOST_CHECK(unknown.permitephemeral_anchor);
    BOOST_CHECK(!unknown.permitephemeral_send);
    BOOST_CHECK(!unknown.permitephemeral_dust);
}

BOOST_AUTO_TEST_CASE(MempoolMinRelayAgeParse)
{
    ArgsManager argsman;
    argsman.ForceSetArg("-minrelaycoinblocks", "123");
    argsman.ForceSetArg("-minrelaymaturity", "4");

    kernel::MemPoolOptions opts;
    BOOST_CHECK_EQUAL(opts.minrelaycoinblocks, 0);
    BOOST_CHECK_EQUAL(opts.minrelaymaturity, 0);
    const auto result{ApplyArgsManOptions(argsman, Params(), opts)};
    BOOST_REQUIRE(result);
    BOOST_CHECK_EQUAL(opts.minrelaycoinblocks, 123);
    BOOST_CHECK_EQUAL(opts.minrelaymaturity, 4);

    argsman.ForceSetArg("-minrelaycoinblocks", "-1");
    BOOST_CHECK(!ApplyArgsManOptions(argsman, Params(), opts));

    argsman.ForceSetArg("-minrelaycoinblocks", "0");
    argsman.ForceSetArg("-minrelaymaturity", "-1");
    BOOST_CHECK(!ApplyArgsManOptions(argsman, Params(), opts));
}

BOOST_AUTO_TEST_CASE(MempoolMaxTxLegacySigopsParse)
{
    ArgsManager argsman;
    argsman.ForceSetArg("-maxtxlegacysigops", "1234");

    kernel::MemPoolOptions opts;
    BOOST_CHECK_EQUAL(opts.maxtxlegacysigops, MAX_TX_LEGACY_SIGOPS);
    const auto result{ApplyArgsManOptions(argsman, Params(), opts)};
    BOOST_REQUIRE(result);
    BOOST_CHECK_EQUAL(opts.maxtxlegacysigops, 1234);
}

BOOST_AUTO_TEST_CASE(MempoolCoinAgePriorityCache)
{
    constexpr CAmount input_value{10 * COIN};

    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = CScript() << std::vector<unsigned char>(110, 0);
    tx.vout.emplace_back(input_value, CScript() << OP_TRUE);
    const auto tx_ref{MakeTransactionRef(tx)};

    const int64_t sigops_cost{4};
    const int32_t tx_size{static_cast<int32_t>(GetVirtualTransactionSize(GetTransactionWeight(*tx_ref), sigops_cost, ::nBytesPerSigOp))};
    const unsigned int modified_size{CalculateModifiedSize(*tx_ref, tx_size)};
    BOOST_CHECK_LT(modified_size, static_cast<unsigned int>(tx_size));

    const double starting_priority{ComputePriority2(input_value, modified_size)};
    CTxMemPoolEntry entry{tx_ref, /*fee=*/0, /*time=*/0, /*entry_height=*/100, /*entry_sequence=*/0,
                          CoinAgeCache{.inputs_coin_age = input_value, .in_chain_input_value = input_value},
                          /*spends_coinbase=*/false, /*extra_weight=*/0, sigops_cost, LockPoints{}};

    BOOST_CHECK_EQUAL(entry.GetStartingPriority(), starting_priority);
    BOOST_CHECK_EQUAL(entry.GetPriority(/*currentHeight=*/101), starting_priority);

    const double priority_after_two_blocks{starting_priority + (2.0 * input_value) / modified_size};
    BOOST_CHECK_EQUAL(entry.GetPriority(/*currentHeight=*/103), priority_after_two_blocks);

    entry.UpdateCachedPriority(/*currentHeight=*/103, /*valueInCurrentBlock=*/5 * COIN);
    BOOST_CHECK_EQUAL(entry.GetPriority(/*currentHeight=*/103), priority_after_two_blocks);
    BOOST_CHECK_EQUAL(entry.GetInternalCoinAgeCache().in_chain_input_value, 15 * COIN);
    BOOST_CHECK_EQUAL(entry.GetPriority(/*currentHeight=*/104), priority_after_two_blocks + (15.0 * COIN) / modified_size);
}

BOOST_AUTO_TEST_CASE(MempoolUpdateDependentPriorities)
{
    constexpr CAmount input_value{10 * COIN};

    CMutableTransaction parent_tx;
    parent_tx.vin.resize(1);
    parent_tx.vin[0].scriptSig = CScript() << OP_1;
    parent_tx.vout.emplace_back(input_value, CScript() << OP_TRUE);

    CMutableTransaction child_tx;
    child_tx.vin.resize(1);
    child_tx.vin[0].prevout = COutPoint{parent_tx.GetHash(), 0};
    child_tx.vin[0].scriptSig = CScript() << OP_2;
    child_tx.vout.emplace_back(input_value, CScript() << OP_TRUE);
    const auto child_tx_ref{MakeTransactionRef(child_tx)};
    const unsigned int modified_size{CalculateModifiedSize(*child_tx_ref, GetVirtualTransactionSize(*child_tx_ref))};

    CTxMemPool& test_pool{*Assert(m_node.mempool)};
    TestMemPoolEntryHelper entry;
    TryAddToMempool(test_pool, entry.Fee(0).FromTx(child_tx));

    LOCK2(::cs_main, test_pool.cs);
    const auto child_entry{test_pool.GetIter(child_tx.GetHash())};
    BOOST_REQUIRE(child_entry);
    BOOST_CHECK_EQUAL((*child_entry)->GetPriority(/*currentHeight=*/102), 0);

    const CTransaction parent{parent_tx};
    test_pool.UpdateDependentPriorities(parent, /*nBlockHeight=*/101, /*addToChain=*/true);
    BOOST_CHECK_EQUAL((*child_entry)->GetPriority(/*currentHeight=*/101), 0);
    BOOST_CHECK_EQUAL((*child_entry)->GetInternalCoinAgeCache().in_chain_input_value, input_value);
    BOOST_CHECK_EQUAL((*child_entry)->GetPriority(/*currentHeight=*/102), static_cast<double>(input_value) / modified_size);

    test_pool.UpdateDependentPriorities(parent, /*nBlockHeight=*/102, /*addToChain=*/false);
    BOOST_CHECK_EQUAL((*child_entry)->GetInternalCoinAgeCache().in_chain_input_value, 0);
    BOOST_CHECK_EQUAL((*child_entry)->GetPriority(/*currentHeight=*/103), static_cast<double>(input_value) / modified_size);
}

BOOST_AUTO_TEST_CASE(MempoolEntryExtraWeight)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    const auto tx_ref{MakeTransactionRef(tx)};

    static constexpr int32_t extra_weight{400};
    static constexpr int64_t sigops_cost{4};
    CTxMemPoolEntry base_entry{tx_ref, /*fee=*/0, /*time=*/0, /*entry_height=*/1, /*entry_sequence=*/0,
                               COIN_AGE_CACHE_ZERO, /*spends_coinbase=*/false, /*extra_weight=*/0,
                               sigops_cost, LockPoints{}};
    CTxMemPoolEntry weighted_entry{tx_ref, /*fee=*/0, /*time=*/0, /*entry_height=*/1, /*entry_sequence=*/0,
                                   COIN_AGE_CACHE_ZERO, /*spends_coinbase=*/false, extra_weight,
                                   sigops_cost, LockPoints{}};

    BOOST_CHECK_EQUAL(weighted_entry.GetTxWeight(), base_entry.GetTxWeight());
    BOOST_CHECK_EQUAL(weighted_entry.GetExtraWeight(), extra_weight);
    BOOST_CHECK_EQUAL(weighted_entry.GetTxSize(), GetVirtualTransactionSize(GetTransactionWeight(*tx_ref) + extra_weight, sigops_cost, ::nBytesPerSigOp));
    BOOST_CHECK_EQUAL(weighted_entry.GetAdjustedWeight(), GetSigOpsAdjustedWeight(GetTransactionWeight(*tx_ref) + extra_weight, sigops_cost, ::nBytesPerSigOp));
    BOOST_CHECK_GT(weighted_entry.GetTxSize(), base_entry.GetTxSize());
    BOOST_CHECK_GT(weighted_entry.GetAdjustedWeight(), base_entry.GetAdjustedWeight());
}

BOOST_AUTO_TEST_CASE(MempoolPriorityAndFeeDeltas)
{
    CMutableTransaction tx;
    tx.vin.emplace_back(Txid::FromUint256(uint256::ONE), 0);
    tx.vout.emplace_back(COIN, CScript() << OP_TRUE);
    const Txid txid{tx.GetHash()};

    CTxMemPool& pool{*Assert(m_node.mempool)};
    TestMemPoolEntryHelper entry;
    TryAddToMempool(pool, entry.Fee(5000).FromTx(tx));

    const unsigned int transactions_updated{pool.GetTransactionsUpdated()};
    constexpr double priority_delta{123.5};
    constexpr CAmount fee_delta{2000};
    pool.PrioritiseTransaction(txid, priority_delta, fee_delta);
    BOOST_CHECK_EQUAL(pool.GetTransactionsUpdated(), transactions_updated + 1);

    {
        LOCK(pool.cs);
        double applied_priority{0.0};
        CAmount applied_fee{0};
        pool.ApplyDeltas(txid, applied_priority, applied_fee);
        BOOST_CHECK_EQUAL(applied_priority, priority_delta);
        BOOST_CHECK_EQUAL(applied_fee, fee_delta);

        const auto iter{pool.GetIter(txid)};
        BOOST_REQUIRE(iter);
        BOOST_CHECK_EQUAL((*iter)->GetModifiedFee(), 7000);
    }

    const auto prioritised{pool.GetPrioritisedTransactions()};
    BOOST_REQUIRE_EQUAL(prioritised.size(), 1);
    BOOST_CHECK(prioritised.front().in_mempool);
    BOOST_CHECK_EQUAL(prioritised.front().priority_delta, priority_delta);
    BOOST_CHECK_EQUAL(prioritised.front().fee_delta, fee_delta);
    BOOST_REQUIRE(prioritised.front().modified_fee);
    BOOST_CHECK_EQUAL(*prioritised.front().modified_fee, 7000);
    BOOST_CHECK(prioritised.front().txid == txid);

    pool.PrioritiseTransaction(txid, -priority_delta, -fee_delta);
    BOOST_CHECK(pool.GetPrioritisedTransactions().empty());
    {
        LOCK(pool.cs);
        double applied_priority{0.0};
        CAmount applied_fee{0};
        pool.ApplyDeltas(txid, applied_priority, applied_fee);
        BOOST_CHECK_EQUAL(applied_priority, 0.0);
        BOOST_CHECK_EQUAL(applied_fee, 0);

        const auto iter{pool.GetIter(txid)};
        BOOST_REQUIRE(iter);
        BOOST_CHECK_EQUAL((*iter)->GetModifiedFee(), 5000);
    }
}

BOOST_AUTO_TEST_CASE(MempoolRemoveTest)
{
    // Test CTxMemPool::remove functionality

    TestMemPoolEntryHelper entry;
    // Parent transaction with three children,
    // and three grand-children:
    CMutableTransaction txParent;
    txParent.vin.resize(1);
    txParent.vin[0].scriptSig = CScript() << OP_11;
    txParent.vout.resize(3);
    for (int i = 0; i < 3; i++)
    {
        txParent.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txParent.vout[i].nValue = 33000LL;
    }
    CMutableTransaction txChild[3];
    for (int i = 0; i < 3; i++)
    {
        txChild[i].vin.resize(1);
        txChild[i].vin[0].scriptSig = CScript() << OP_11;
        txChild[i].vin[0].prevout.hash = txParent.GetHash();
        txChild[i].vin[0].prevout.n = i;
        txChild[i].vout.resize(1);
        txChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txChild[i].vout[0].nValue = 11000LL;
    }
    CMutableTransaction txGrandChild[3];
    for (int i = 0; i < 3; i++)
    {
        txGrandChild[i].vin.resize(1);
        txGrandChild[i].vin[0].scriptSig = CScript() << OP_11;
        txGrandChild[i].vin[0].prevout.hash = txChild[i].GetHash();
        txGrandChild[i].vin[0].prevout.n = 0;
        txGrandChild[i].vout.resize(1);
        txGrandChild[i].vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        txGrandChild[i].vout[0].nValue = 11000LL;
    }


    CTxMemPool& testPool = *Assert(m_node.mempool);
    LOCK2(::cs_main, testPool.cs);

    // Nothing in pool, remove should do nothing:
    unsigned int poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);

    // Just the parent:
    TryAddToMempool(testPool, entry.FromTx(txParent));
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 1);

    // Parent, children, grandchildren:
    TryAddToMempool(testPool, entry.FromTx(txParent));
    for (int i = 0; i < 3; i++)
    {
        TryAddToMempool(testPool, entry.FromTx(txChild[i]));
        TryAddToMempool(testPool, entry.FromTx(txGrandChild[i]));
    }
    // Remove Child[0], GrandChild[0] should be removed:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 2);
    // ... make sure grandchild and child are gone:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txGrandChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txChild[0]), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize);
    // Remove parent, all children/grandchildren should go:
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 5);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);

    // Add children and grandchildren, but NOT the parent (simulate the parent being in a block)
    for (int i = 0; i < 3; i++)
    {
        TryAddToMempool(testPool, entry.FromTx(txChild[i]));
        TryAddToMempool(testPool, entry.FromTx(txGrandChild[i]));
    }
    // Now remove the parent, as might happen if a block-re-org occurs but the parent cannot be
    // put into the mempool (maybe because it is non-standard):
    poolSize = testPool.size();
    testPool.removeRecursive(CTransaction(txParent), REMOVAL_REASON_DUMMY);
    BOOST_CHECK_EQUAL(testPool.size(), poolSize - 6);
    BOOST_CHECK_EQUAL(testPool.size(), 0U);
}

BOOST_AUTO_TEST_CASE(MempoolSizeLimitTest)
{
    auto& pool = static_cast<MemPoolTest&>(*Assert(m_node.mempool));
    LOCK2(cs_main, pool.cs);
    pool.m_opts.min_relay_feerate = CFeeRate{CORE_INCREMENTAL_RELAY_FEE};
    pool.m_opts.incremental_relay_feerate = CFeeRate{CORE_INCREMENTAL_RELAY_FEE};
    TestMemPoolEntryHelper entry;

    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vin.resize(1);
    tx1.vin[0].scriptSig = CScript() << OP_1;
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_1 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    TryAddToMempool(pool, entry.Fee(1000LL).FromTx(tx1));

    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vin.resize(1);
    tx2.vin[0].scriptSig = CScript() << OP_2;
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_2 << OP_EQUAL;
    tx2.vout[0].nValue = 10 * COIN;
    TryAddToMempool(pool, entry.Fee(500LL).FromTx(tx2));

    pool.TrimToSize(pool.DynamicMemoryUsage()); // should do nothing
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // should remove the lower-feerate transaction
    BOOST_CHECK(pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));

    TryAddToMempool(pool, entry.FromTx(tx2));
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vin.resize(1);
    tx3.vin[0].prevout = COutPoint(tx2.GetHash(), 0);
    tx3.vin[0].scriptSig = CScript() << OP_2;
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_3 << OP_EQUAL;
    tx3.vout[0].nValue = 10 * COIN;
    TryAddToMempool(pool, entry.Fee(2000LL).FromTx(tx3));

    pool.TrimToSize(pool.DynamicMemoryUsage() * 3 / 4); // tx3 should pay for tx2 (CPFP)
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(pool.exists(tx2.GetHash()));
    BOOST_CHECK(pool.exists(tx3.GetHash()));

    pool.TrimToSize(GetVirtualTransactionSize(CTransaction(tx1))); // mempool is limited to tx1's size in memory usage, so nothing fits
    BOOST_CHECK(!pool.exists(tx1.GetHash()));
    BOOST_CHECK(!pool.exists(tx2.GetHash()));
    BOOST_CHECK(!pool.exists(tx3.GetHash()));

    CFeeRate maxFeeRateRemoved(2500, GetVirtualTransactionSize(CTransaction(tx3)) + GetVirtualTransactionSize(CTransaction(tx2)));
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + pool.m_opts.incremental_relay_feerate.GetFeePerK());

    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vin.resize(2);
    tx4.vin[0].prevout.SetNull();
    tx4.vin[0].scriptSig = CScript() << OP_4;
    tx4.vin[1].prevout.SetNull();
    tx4.vin[1].scriptSig = CScript() << OP_4;
    tx4.vout.resize(2);
    tx4.vout[0].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[0].nValue = 10 * COIN;
    tx4.vout[1].scriptPubKey = CScript() << OP_4 << OP_EQUAL;
    tx4.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vin.resize(2);
    tx5.vin[0].prevout = COutPoint(tx4.GetHash(), 0);
    tx5.vin[0].scriptSig = CScript() << OP_4;
    tx5.vin[1].prevout.SetNull();
    tx5.vin[1].scriptSig = CScript() << OP_5;
    tx5.vout.resize(2);
    tx5.vout[0].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[0].nValue = 10 * COIN;
    tx5.vout[1].scriptPubKey = CScript() << OP_5 << OP_EQUAL;
    tx5.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx6 = CMutableTransaction();
    tx6.vin.resize(2);
    tx6.vin[0].prevout = COutPoint(tx4.GetHash(), 1);
    tx6.vin[0].scriptSig = CScript() << OP_4;
    tx6.vin[1].prevout.SetNull();
    tx6.vin[1].scriptSig = CScript() << OP_6;
    tx6.vout.resize(2);
    tx6.vout[0].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[0].nValue = 10 * COIN;
    tx6.vout[1].scriptPubKey = CScript() << OP_6 << OP_EQUAL;
    tx6.vout[1].nValue = 10 * COIN;

    CMutableTransaction tx7 = CMutableTransaction();
    tx7.vin.resize(2);
    tx7.vin[0].prevout = COutPoint(tx5.GetHash(), 0);
    tx7.vin[0].scriptSig = CScript() << OP_5;
    tx7.vin[1].prevout = COutPoint(tx6.GetHash(), 0);
    tx7.vin[1].scriptSig = CScript() << OP_6;
    tx7.vout.resize(2);
    tx7.vout[0].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[0].nValue = 10 * COIN;
    tx7.vout[1].scriptPubKey = CScript() << OP_7 << OP_EQUAL;
    tx7.vout[1].nValue = 10 * COIN;

    TryAddToMempool(pool, entry.Fee(700LL).FromTx(tx4));
    TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    // From the topology above, tx7 must be sorted last, so it should
    // definitely evicted first if we must trim. tx4 should definitely remain
    // in the mempool since it has a higher feerate than its descendants and
    // should be in its own chunk.
    pool.TrimToSize(pool.DynamicMemoryUsage() - 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    if (!pool.exists(tx5.GetHash()))
        TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    if (!pool.exists(tx5.GetHash()))
        TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    if (!pool.exists(tx6.GetHash()))
        TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    std::vector<CTransactionRef> vtx;
    FakeNodeClock clock{42s};
    constexpr std::chrono::seconds HALFLIFE{CTxMemPool::ROLLING_FEE_HALFLIFE};
    const CAmount incremental_relay_fee{pool.m_opts.incremental_relay_feerate.GetFeePerK()};
    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + incremental_relay_fee);
    // ... we should keep the same min fee until we get a block
    pool.removeForBlock(vtx, 1);
    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + incremental_relay_fee)/2.0));
    // ... then feerate should drop 1/2 each halflife

    clock += HALFLIFE / 2;
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 5 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + incremental_relay_fee)/4.0));
    // ... with a 1/2 halflife when mempool is < 1/2 its target size

    clock += HALFLIFE / 4;
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 9 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + incremental_relay_fee)/8.0));
    // ... with a 1/4 halflife when mempool is < 1/4 its target size

    clock += 5 * HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), incremental_relay_fee);
    // ... but feerate should never drop below DEFAULT_INCREMENTAL_RELAY_FEE

    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), 0);
    // ... unless it has gone all the way to 0 (after getting past DEFAULT_INCREMENTAL_RELAY_FEE/2)
}

inline CTransactionRef make_tx(std::vector<CAmount>&& output_values, std::vector<CTransactionRef>&& inputs=std::vector<CTransactionRef>(), std::vector<uint32_t>&& input_indices=std::vector<uint32_t>())
{
    CMutableTransaction tx = CMutableTransaction();
    tx.vin.resize(inputs.size());
    tx.vout.resize(output_values.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        tx.vin[i].prevout.hash = inputs[i]->GetHash();
        tx.vin[i].prevout.n = input_indices.size() > i ? input_indices[i] : 0;
    }
    for (size_t i = 0; i < output_values.size(); ++i) {
        tx.vout[i].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[i].nValue = output_values[i];
    }
    return MakeTransactionRef(tx);
}


BOOST_AUTO_TEST_CASE(MempoolAncestryTests)
{
    size_t ancestors, clustersize;

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Base transaction */
    //
    // [tx1]
    //
    CTransactionRef tx1 = make_tx(/*output_values=*/{10 * COIN});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx1));

    // Ancestors / clustersize should be 1 / 1 (itself / itself)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 1ULL);

    /* Child transaction */
    //
    // [tx1].0 <- [tx2]
    //
    CTransactionRef tx2 = make_tx(/*output_values=*/{495 * CENT, 5 * COIN}, /*inputs=*/{tx1});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx2));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     2 (tx1,2)
    // tx2          2 (tx1,2)   2 (tx1,2)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 2ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 2ULL);

    /* Grand-child 1 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //
    CTransactionRef tx3 = make_tx(/*output_values=*/{290 * CENT, 200 * CENT}, /*inputs=*/{tx2});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx3));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     3 (tx1,2,3)
    // tx2          2 (tx1,2)   3 (tx1,2,3)
    // tx3          3 (tx1,2,3) 3 (tx1,2,3)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 3ULL);

    /* Grand-child 2 */
    //
    // [tx1].0 <- [tx2].0 <- [tx3]
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef tx4 = make_tx(/*output_values=*/{290 * CENT, 250 * CENT}, /*inputs=*/{tx2}, /*input_indices=*/{1});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx4));

    // Ancestors / clustersize should be:
    // transaction  ancestors   clustersize
    // ============ =========== ===========
    // tx1          1 (tx1)     4 (tx1,2,3,4)
    // tx2          2 (tx1,2)   4 (tx1,2,3,4)
    // tx3          3 (tx1,2,3) 4 (tx1,2,3,4)
    // tx4          3 (tx1,2,4) 4 (tx1,2,3,4)
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 4ULL);

    /* Make an alternate branch that is longer and connect it to tx3 */
    //
    // [ty1].0 <- [ty2].0 <- [ty3].0 <- [ty4].0 <- [ty5].0
    //                                              |
    // [tx1].0 <- [tx2].0 <- [tx3].0 <- [ty6] --->--/
    //              |
    //              \---1 <- [tx4]
    //
    CTransactionRef ty1, ty2, ty3, ty4, ty5;
    CTransactionRef* ty[5] = {&ty1, &ty2, &ty3, &ty4, &ty5};
    CAmount v = 5 * COIN;
    for (uint64_t i = 0; i < 5; i++) {
        CTransactionRef& tyi = *ty[i];
        tyi = make_tx(/*output_values=*/{v}, /*inputs=*/i > 0 ? std::vector<CTransactionRef>{*ty[i - 1]} : std::vector<CTransactionRef>{});
        v -= 50 * CENT;
        TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tyi));
        pool.GetTransactionAncestry(tyi->GetHash(), ancestors, clustersize);
        BOOST_CHECK_EQUAL(ancestors, i+1);
        BOOST_CHECK_EQUAL(clustersize, i+1);
    }
    CTransactionRef ty6 = make_tx(/*output_values=*/{5 * COIN}, /*inputs=*/{tx3, ty5});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(ty6));

    // Ancestors / clustersize should be:
    // transaction  ancestors           clustersize
    // ============ =================== ===========
    // tx1          1 (tx1)             10 (tx1-5, ty1-5)
    // tx2          2 (tx1,2)           10
    // tx3          3 (tx1,2,3)         10
    // tx4          3 (tx1,2,4)         10
    // ty1          1 (ty1)             10
    // ty2          2 (ty1,2)           10
    // ty3          3 (ty1,2,3)         10
    // ty4          4 (y1234)           10
    // ty5          5 (y12345)          10
    // ty6          9 (tx123, ty123456) 10
    pool.GetTransactionAncestry(tx1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(tx4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty1->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty2->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty3->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty4->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty5->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 5ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
    pool.GetTransactionAncestry(ty6->GetHash(), ancestors, clustersize);
    BOOST_CHECK_EQUAL(ancestors, 9ULL);
    BOOST_CHECK_EQUAL(clustersize, 10ULL);
}

BOOST_AUTO_TEST_CASE(MempoolAncestryTestsDiamond)
{
    size_t ancestors, descendants;

    CTxMemPool& pool = *Assert(m_node.mempool);
    LOCK2(::cs_main, pool.cs);
    TestMemPoolEntryHelper entry;

    /* Ancestors represented more than once ("diamond") */
    //
    // [ta].0 <- [tb].0 -----<------- [td].0
    //            |                    |
    //            \---1 <- [tc].0 --<--/
    //
    CTransactionRef ta, tb, tc, td;
    ta = make_tx(/*output_values=*/{10 * COIN});
    tb = make_tx(/*output_values=*/{5 * COIN, 3 * COIN}, /*inputs=*/ {ta});
    tc = make_tx(/*output_values=*/{2 * COIN}, /*inputs=*/{tb}, /*input_indices=*/{1});
    td = make_tx(/*output_values=*/{6 * COIN}, /*inputs=*/{tb, tc}, /*input_indices=*/{0, 0});
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(ta));
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tb));
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tc));
    TryAddToMempool(pool, entry.Fee(10000LL).FromTx(td));

    // Ancestors / descendants should be:
    // transaction  ancestors           descendants
    // ============ =================== ===========
    // ta           1 (ta               4 (ta,tb,tc,td)
    // tb           2 (ta,tb)           4 (ta,tb,tc,td)
    // tc           3 (ta,tb,tc)        4 (ta,tb,tc,td)
    // td           4 (ta,tb,tc,td)     4 (ta,tb,tc,td)
    pool.GetTransactionAncestry(ta->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 1ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(tb->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 2ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(tc->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 3ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
    pool.GetTransactionAncestry(td->GetHash(), ancestors, descendants);
    BOOST_CHECK_EQUAL(ancestors, 4ULL);
    BOOST_CHECK_EQUAL(descendants, 4ULL);
}

BOOST_AUTO_TEST_SUITE_END()
