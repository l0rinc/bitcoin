// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/system.h>
#include <addresstype.h>
#include <node/mempool_persist.h>
#include <policy/policy.h>
#include <streams.h>
#include <test/util/time.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <util/time.h>
#include <validation.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>
#include <cstddef>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(mempool_tests, TestingSetup)

static constexpr auto REMOVAL_REASON_DUMMY = MemPoolRemovalReason::REPLACED;

class MemPoolTest final : public CTxMemPool
{
public:
    using CTxMemPool::GetMinFee;
};

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
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE);

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
    auto usage_with_tx4_only = pool.DynamicMemoryUsage();
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

    // Tx5 and Tx6 may be removed as well because they're in the same chunk as
    // tx7, but this behavior need not be guaranteed.

    if (!pool.exists(tx5.GetHash()))
        TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    if (!pool.exists(tx6.GetHash()))
        TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    // If we trim sufficiently, everything but tx4 should be removed.
    pool.TrimToSize(usage_with_tx4_only + 1);
    BOOST_CHECK(pool.exists(tx4.GetHash()));
    BOOST_CHECK(!pool.exists(tx5.GetHash()));
    BOOST_CHECK(!pool.exists(tx6.GetHash()));
    BOOST_CHECK(!pool.exists(tx7.GetHash()));

    TryAddToMempool(pool, entry.Fee(100LL).FromTx(tx5));
    TryAddToMempool(pool, entry.Fee(110LL).FromTx(tx6));
    TryAddToMempool(pool, entry.Fee(900LL).FromTx(tx7));

    std::vector<CTransactionRef> vtx;
    FakeNodeClock clock{42s};
    constexpr std::chrono::seconds HALFLIFE{CTxMemPool::ROLLING_FEE_HALFLIFE};
    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE);
    // ... we should keep the same min fee until we get a block
    pool.removeForBlock(vtx, 1);
    clock += HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/2.0));
    // ... then feerate should drop 1/2 each halflife

    clock += HALFLIFE / 2;
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 5 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/4.0));
    // ... with a 1/2 halflife when mempool is < 1/2 its target size

    clock += HALFLIFE / 4;
    BOOST_CHECK_EQUAL(pool.GetMinFee(pool.DynamicMemoryUsage() * 9 / 2).GetFeePerK(), llround((maxFeeRateRemoved.GetFeePerK() + DEFAULT_INCREMENTAL_RELAY_FEE)/8.0));
    // ... with a 1/4 halflife when mempool is < 1/4 its target size

    clock += 5 * HALFLIFE;
    BOOST_CHECK_EQUAL(pool.GetMinFee(1).GetFeePerK(), DEFAULT_INCREMENTAL_RELAY_FEE);
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

BOOST_AUTO_TEST_CASE(MempoolPrioritisationSaturationRoundTrip)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    const CTransactionRef tx = make_tx({10 * COIN});
    constexpr CAmount BASE_FEE{1000};
    constexpr CAmount MAX_DELTA{std::numeric_limits<CAmount>::max()};

    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(BASE_FEE).FromTx(tx));
    }

    pool.PrioritiseTransaction(tx->GetHash(), MAX_DELTA);
    pool.PrioritiseTransaction(tx->GetHash(), -MAX_DELTA);

    {
        LOCK(pool.cs);
        const auto* current = pool.GetEntry(tx->GetHash());
        BOOST_REQUIRE(current != nullptr);
        BOOST_CHECK_EQUAL(current->GetModifiedFee(), BASE_FEE);
        BOOST_CHECK(pool.mapDeltas.find(tx->GetHash()) == pool.mapDeltas.end());
    }

    {
        LOCK2(::cs_main, pool.cs);
        pool.removeRecursive(*tx, REMOVAL_REASON_DUMMY);
        TryAddToMempool(pool, entry.Fee(BASE_FEE).FromTx(tx));
        const auto* current = pool.GetEntry(tx->GetHash());
        BOOST_REQUIRE(current != nullptr);
        BOOST_CHECK_EQUAL(current->GetModifiedFee(), BASE_FEE);
    }
}

BOOST_AUTO_TEST_CASE(MempoolUnbroadcastMemoryAccounting)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    const CTransactionRef tx = make_tx({10 * COIN});
    {
        LOCK2(::cs_main, pool.cs);
        TryAddToMempool(pool, entry.Fee(10000LL).FromTx(tx));
    }

    const size_t usage_before = pool.DynamicMemoryUsage();
    pool.AddUnbroadcastTx(tx->GetHash());
    const size_t usage_with_unbroadcast = pool.DynamicMemoryUsage();
    BOOST_CHECK(pool.GetUnbroadcastTxs().contains(tx->GetHash()));
    BOOST_CHECK_GT(usage_with_unbroadcast, usage_before);

    pool.RemoveUnbroadcastTx(tx->GetHash());
    BOOST_CHECK(!pool.GetUnbroadcastTxs().contains(tx->GetHash()));
    BOOST_CHECK_EQUAL(pool.DynamicMemoryUsage(), usage_before);
}

BOOST_FIXTURE_TEST_CASE(MempoolV1SignedDeltaExtremes, TestChain100Setup)
{
    bilingual_str error;
    CTxMemPool::Options source_options{MemPoolOptionsForTest(m_node)};
    source_options.persist_v1_dat = true;
    CTxMemPool source_pool{source_options, error};
    BOOST_REQUIRE(error.empty());

    const CTransactionRef tx = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef absent_tx = make_tx({1 * COIN});
    TestMemPoolEntryHelper entry;
    TryAddToMempool(source_pool, entry.Fee(COIN).Time(Now<NodeSeconds>()).FromTx(tx));
    source_pool.PrioritiseTransaction(tx->GetHash(), std::numeric_limits<CAmount>::max());
    source_pool.PrioritiseTransaction(absent_tx->GetHash(), std::numeric_limits<CAmount>::min());

    const fs::path dump_path = m_path_root / "mempool-v1-signed-extremes.dat";
    BOOST_REQUIRE(node::DumpMempool(source_pool, dump_path, fsbridge::fopen, true));

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = false,
    }));
    BOOST_REQUIRE(destination.exists(tx->GetHash()));
    const auto tx_info = destination.info(tx->GetHash());
    BOOST_CHECK(tx_info.fee + tx_info.nFeeDelta == std::numeric_limits<CAmount>::max());

    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 2U);
    for (const auto& delta : deltas) {
        if (delta.txid == tx->GetHash()) BOOST_CHECK_EQUAL(delta.delta, std::numeric_limits<CAmount>::max());
        if (delta.txid == absent_tx->GetHash()) BOOST_CHECK_EQUAL(delta.delta, std::numeric_limits<CAmount>::min());
    }

    source_pool.PrioritiseTransaction(tx->GetHash(), -COIN);
    destination.PrioritiseTransaction(tx->GetHash(), -COIN);
    CAmount source_modified_fee;
    {
        LOCK(source_pool.cs);
        source_modified_fee = Assert(source_pool.GetEntry(tx->GetHash()))->GetModifiedFee();
    }
    const auto destination_info = destination.info(tx->GetHash());
    BOOST_CHECK_EQUAL(source_modified_fee, destination_info.fee + destination_info.nFeeDelta);

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1DuplicateTransactionRecords, TestChain100Setup)
{
    const CTransactionRef tx = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const fs::path dump_path = m_path_root / "mempool-v1-duplicate-records.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());

    DataStream dump;
    dump << uint64_t{1} << uint64_t{2};
    dump << TX_WITH_WITNESS(*tx) << now << int64_t{1000};
    dump << TX_WITH_WITNESS(*tx) << now << int64_t{2000};
    dump << std::map<Txid, CAmount>{} << std::set<Txid>{};

    std::ofstream file{dump_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = false,
    }));
    const auto tx_info = destination.info(tx->GetHash());
    BOOST_REQUIRE_EQUAL(destination.size(), 1U);
    BOOST_CHECK_EQUAL(tx_info.nFeeDelta, 1000);

    DataStream existing_dump;
    existing_dump << uint64_t{1} << uint64_t{1};
    existing_dump << TX_WITH_WITNESS(*tx) << now << int64_t{4000};
    existing_dump << std::map<Txid, CAmount>{} << std::set<Txid>{};
    std::ofstream existing_file{dump_path.std_path(), std::ios::binary};
    existing_file.write(reinterpret_cast<const char*>(existing_dump.data()), existing_dump.size());
    existing_file.close();
    BOOST_REQUIRE(existing_file.good());

    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = false,
    }));
    BOOST_CHECK_EQUAL(destination.info(tx->GetHash()).nFeeDelta, 5000);

    DataStream trailing_map_dump;
    trailing_map_dump << uint64_t{1} << uint64_t{1};
    trailing_map_dump << TX_WITH_WITNESS(*tx) << now << int64_t{1000};
    trailing_map_dump << std::map<Txid, CAmount>{{tx->GetHash(), 2000}} << std::set<Txid>{};
    std::ofstream trailing_map_file{dump_path.std_path(), std::ios::binary};
    trailing_map_file.write(reinterpret_cast<const char*>(trailing_map_dump.data()), trailing_map_dump.size());
    trailing_map_file.close();
    BOOST_REQUIRE(trailing_map_file.good());

    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = false,
    }));
    BOOST_CHECK_EQUAL(destination.info(tx->GetHash()).nFeeDelta, 6000);

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1PartialImportMetadata, TestChain100Setup)
{
    mineBlocks(1);
    const CTransactionRef tx_before = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef tx_after = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CMutableTransaction invalid_mutable;
    invalid_mutable.vin.resize(1);
    invalid_mutable.vin[0].prevout.SetNull();
    invalid_mutable.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    const CTransactionRef invalid_tx = MakeTransactionRef(invalid_mutable);

    const fs::path dump_path = m_path_root / "mempool-v1-partial-import-metadata.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    DataStream dump;
    dump << uint64_t{1} << uint64_t{3};
    dump << TX_WITH_WITNESS(*tx_before) << now << int64_t{1000};
    dump << TX_WITH_WITNESS(*invalid_tx) << now << int64_t{2000};
    dump << TX_WITH_WITNESS(*tx_after) << now << int64_t{3000};
    dump << std::map<Txid, CAmount>{}
         << std::set<Txid>{tx_before->GetHash(), invalid_tx->GetHash(), tx_after->GetHash()};

    std::ofstream file{dump_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(invalid_tx->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(tx_before->GetHash()).nFeeDelta, 1000);
    BOOST_CHECK_EQUAL(destination.info(tx_after->GetHash()).nFeeDelta, 3000);

    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 3U);
    for (const auto& delta : deltas) {
        if (delta.txid == tx_before->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 1000);
        } else if (delta.txid == invalid_tx->GetHash()) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 2000);
        } else if (delta.txid == tx_after->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 3000);
        } else {
            BOOST_FAIL("unexpected prioritization entry");
        }
    }

    const std::set<Txid> expected_unbroadcast{tx_before->GetHash(), tx_after->GetHash()};
    BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
        destination.ClearPrioritisation(invalid_tx->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1PartialImportTrailingMetadata, TestChain100Setup)
{
    mineBlocks(1);
    const CTransactionRef tx_before = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef tx_after = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CMutableTransaction invalid_mutable;
    invalid_mutable.vin.resize(1);
    invalid_mutable.vin[0].prevout.SetNull();
    invalid_mutable.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    const CTransactionRef invalid_tx = MakeTransactionRef(invalid_mutable);
    const Txid absent_txid = Txid::FromUint256(uint256{42});

    const fs::path dump_path = m_path_root / "mempool-v1-partial-import-trailing-metadata.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    DataStream dump;
    dump << uint64_t{1} << uint64_t{3};
    dump << TX_WITH_WITNESS(*tx_before) << now << int64_t{1000};
    dump << TX_WITH_WITNESS(*invalid_tx) << now << int64_t{2000};
    dump << TX_WITH_WITNESS(*tx_after) << now << int64_t{3000};
    dump << std::map<Txid, CAmount>{
               {tx_before->GetHash(), 1100},
               {invalid_tx->GetHash(), 2200},
               {tx_after->GetHash(), 3300},
               {absent_txid, 4400}}
         << std::set<Txid>{tx_before->GetHash(), invalid_tx->GetHash(), tx_after->GetHash(), absent_txid};

    std::ofstream file{dump_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(invalid_tx->GetHash()));

    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 4U);
    for (const auto& delta : deltas) {
        if (delta.txid == tx_before->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 1000);
        } else if (delta.txid == invalid_tx->GetHash()) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 2000);
        } else if (delta.txid == tx_after->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 3000);
        } else if (delta.txid == absent_txid) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 4400);
        } else {
            BOOST_FAIL("unexpected prioritization entry");
        }
    }

    const std::set<Txid> expected_unbroadcast{tx_before->GetHash(), tx_after->GetHash()};
    BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
        destination.ClearPrioritisation(invalid_tx->GetHash());
        destination.ClearPrioritisation(absent_txid);
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1ZeroFeeDeltaMetadata, TestChain100Setup)
{
    mineBlocks(1);
    const CTransactionRef tx_before = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef tx_after = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CMutableTransaction invalid_mutable;
    invalid_mutable.vin.resize(1);
    invalid_mutable.vin[0].prevout.SetNull();
    invalid_mutable.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    const CTransactionRef invalid_tx = MakeTransactionRef(invalid_mutable);
    const Txid absent_txid = Txid::FromUint256(uint256{43});

    const fs::path dump_path = m_path_root / "mempool-v1-zero-fee-delta-metadata.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    DataStream dump;
    dump << uint64_t{1} << uint64_t{3};
    dump << TX_WITH_WITNESS(*tx_before) << now << int64_t{0};
    dump << TX_WITH_WITNESS(*invalid_tx) << now << int64_t{0};
    dump << TX_WITH_WITNESS(*tx_after) << now << int64_t{0};
    dump << std::map<Txid, CAmount>{
               {tx_before->GetHash(), 0},
               {invalid_tx->GetHash(), 0},
               {tx_after->GetHash(), 0},
               {absent_txid, 0}}
         << std::set<Txid>{tx_before->GetHash(), invalid_tx->GetHash(), tx_after->GetHash(), absent_txid};

    std::ofstream file{dump_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(invalid_tx->GetHash()));
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());

    const std::set<Txid> expected_unbroadcast{tx_before->GetHash(), tx_after->GetHash()};
    BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
        destination.ClearPrioritisation(invalid_tx->GetHash());
        destination.ClearPrioritisation(absent_txid);
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1SignedPartialImportMetadata, TestChain100Setup)
{
    mineBlocks(1);
    const CTransactionRef tx_before = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef tx_after = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CMutableTransaction invalid_mutable;
    invalid_mutable.vin.resize(1);
    invalid_mutable.vin[0].prevout.SetNull();
    invalid_mutable.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    const CTransactionRef invalid_tx = MakeTransactionRef(invalid_mutable);
    const Txid absent_txid = Txid::FromUint256(uint256{44});

    const fs::path dump_path = m_path_root / "mempool-v1-signed-partial-import-metadata.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    DataStream dump;
    dump << uint64_t{1} << uint64_t{3};
    dump << TX_WITH_WITNESS(*tx_before) << now << int64_t{-1000};
    dump << TX_WITH_WITNESS(*invalid_tx) << now << int64_t{-2000};
    dump << TX_WITH_WITNESS(*tx_after) << now << int64_t{-3000};
    dump << std::map<Txid, CAmount>{
               {tx_before->GetHash(), -1100},
               {invalid_tx->GetHash(), -2200},
               {tx_after->GetHash(), -3300},
               {absent_txid, -4400}}
         << std::set<Txid>{tx_before->GetHash(), invalid_tx->GetHash(), tx_after->GetHash(), absent_txid};

    std::ofstream file{dump_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(invalid_tx->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(tx_before->GetHash()).nFeeDelta, -1000);
    BOOST_CHECK_EQUAL(destination.info(tx_after->GetHash()).nFeeDelta, -3000);

    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 4U);
    for (const auto& delta : deltas) {
        if (delta.txid == tx_before->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -1000);
        } else if (delta.txid == invalid_tx->GetHash()) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -2000);
        } else if (delta.txid == tx_after->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -3000);
        } else if (delta.txid == absent_txid) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -4400);
        } else {
            BOOST_FAIL("unexpected prioritization entry");
        }
    }

    const std::set<Txid> expected_unbroadcast{tx_before->GetHash(), tx_after->GetHash()};
    BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
        destination.ClearPrioritisation(invalid_tx->GetHash());
        destination.ClearPrioritisation(absent_txid);
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1SignedPartialImportDumpReload, TestChain100Setup)
{
    mineBlocks(1);
    const CTransactionRef tx_before = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef tx_after = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CMutableTransaction invalid_mutable;
    invalid_mutable.vin.resize(1);
    invalid_mutable.vin[0].prevout.SetNull();
    invalid_mutable.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    const CTransactionRef invalid_tx = MakeTransactionRef(invalid_mutable);
    const Txid absent_txid = Txid::FromUint256(uint256{45});

    const fs::path import_path = m_path_root / "mempool-v1-signed-partial-import-roundtrip.dat";
    const fs::path roundtrip_path = m_path_root / "mempool-signed-partial-import-roundtrip.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    DataStream dump;
    dump << uint64_t{1} << uint64_t{3};
    dump << TX_WITH_WITNESS(*tx_before) << now << int64_t{-1000};
    dump << TX_WITH_WITNESS(*invalid_tx) << now << int64_t{-2000};
    dump << TX_WITH_WITNESS(*tx_after) << now << int64_t{-3000};
    dump << std::map<Txid, CAmount>{
               {tx_before->GetHash(), -1100},
               {invalid_tx->GetHash(), -2200},
               {tx_after->GetHash(), -3300},
               {absent_txid, -4400}}
         << std::set<Txid>{tx_before->GetHash(), invalid_tx->GetHash(), tx_after->GetHash(), absent_txid};

    std::ofstream file{import_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, import_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(invalid_tx->GetHash()));

    BOOST_REQUIRE(node::DumpMempool(destination, roundtrip_path, fsbridge::fopen, true));
    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
        destination.ClearPrioritisation(invalid_tx->GetHash());
        destination.ClearPrioritisation(absent_txid);
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(node::LoadMempool(destination, roundtrip_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(invalid_tx->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(tx_before->GetHash()).nFeeDelta, -1000);
    BOOST_CHECK_EQUAL(destination.info(tx_after->GetHash()).nFeeDelta, -3000);

    const auto roundtrip_deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(roundtrip_deltas.size(), 4U);
    for (const auto& delta : roundtrip_deltas) {
        if (delta.txid == tx_before->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -1000);
        } else if (delta.txid == invalid_tx->GetHash()) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -2000);
        } else if (delta.txid == tx_after->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -3000);
        } else if (delta.txid == absent_txid) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, -4400);
        } else {
            BOOST_FAIL("unexpected prioritization entry");
        }
    }

    const std::set<Txid> expected_unbroadcast{tx_before->GetHash(), tx_after->GetHash()};
    BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
        destination.ClearPrioritisation(invalid_tx->GetHash());
        destination.ClearPrioritisation(absent_txid);
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(fs::remove(import_path));
    BOOST_REQUIRE(fs::remove(roundtrip_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1ConflictingPartialImportMetadata, TestChain100Setup)
{
    mineBlocks(1);
    const CTransactionRef tx_before = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    const CTransactionRef tx_after = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CMutableTransaction conflicting_mutable{*tx_before};
    conflicting_mutable.vout[0].nValue -= 1;
    const CTransactionRef conflicting_tx = MakeTransactionRef(conflicting_mutable);
    BOOST_REQUIRE(conflicting_tx->GetHash() != tx_before->GetHash());
    BOOST_REQUIRE(conflicting_tx->vin[0].prevout == tx_before->vin[0].prevout);

    const fs::path dump_path = m_path_root / "mempool-v1-conflicting-partial-import-metadata.dat";
    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    DataStream dump;
    dump << uint64_t{1} << uint64_t{3};
    dump << TX_WITH_WITNESS(*tx_before) << now << int64_t{4000};
    dump << TX_WITH_WITNESS(*conflicting_tx) << now << int64_t{5000};
    dump << TX_WITH_WITNESS(*tx_after) << now << int64_t{6000};
    dump << std::map<Txid, CAmount>{}
         << std::set<Txid>{tx_before->GetHash(), conflicting_tx->GetHash(), tx_after->GetHash()};

    std::ofstream file{dump_path.std_path(), std::ios::binary};
    file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
    file.close();
    BOOST_REQUIRE(file.good());

    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_REQUIRE_EQUAL(destination.size(), 2U);
    BOOST_CHECK(destination.exists(tx_before->GetHash()));
    BOOST_CHECK(destination.exists(tx_after->GetHash()));
    BOOST_CHECK(!destination.exists(conflicting_tx->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(tx_before->GetHash()).nFeeDelta, 4000);
    BOOST_CHECK_EQUAL(destination.info(tx_after->GetHash()).nFeeDelta, 6000);

    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 3U);
    for (const auto& delta : deltas) {
        if (delta.txid == tx_before->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 4000);
        } else if (delta.txid == conflicting_tx->GetHash()) {
            BOOST_CHECK(!delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 5000);
        } else if (delta.txid == tx_after->GetHash()) {
            BOOST_CHECK(delta.in_mempool);
            BOOST_CHECK_EQUAL(delta.delta, 6000);
        } else {
            BOOST_FAIL("unexpected prioritization entry");
        }
    }

    const std::set<Txid> expected_unbroadcast{tx_before->GetHash(), tx_after->GetHash()};
    BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*tx_before), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*tx_after), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(tx_before->GetHash());
        destination.ClearPrioritisation(conflicting_tx->GetHash());
        destination.ClearPrioritisation(tx_after->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());

    BOOST_REQUIRE(fs::remove(dump_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1DependencyOrdering, TestChain100Setup)
{
    mineBlocks(1);

    const CKey parent_key = GenerateRandomKey();
    const CScript parent_destination = GetScriptForDestination(PKHash(parent_key.GetPubKey()));
    const CTransactionRef parent = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[0], 0, 0, coinbaseKey, parent_destination, 49 * COIN, false));

    const CKey child_key = GenerateRandomKey();
    const CScript child_destination = GetScriptForDestination(PKHash(child_key.GetPubKey()));
    const CTransactionRef child = MakeTransactionRef(CreateValidMempoolTransaction(
        parent, 0, 101, parent_key, child_destination, 48 * COIN, false));

    const CTransactionRef independent = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));

    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    const fs::path child_first_path = m_path_root / "mempool-v1-child-first.dat";
    const fs::path parent_first_path = m_path_root / "mempool-v1-parent-first.dat";
    const auto write_dump = [&](const fs::path& path, bool parent_first) {
        DataStream dump;
        dump << uint64_t{1} << uint64_t{3};
        const std::vector<std::pair<CTransactionRef, CAmount>> records = parent_first
            ? std::vector<std::pair<CTransactionRef, CAmount>>{{parent, 1000}, {child, 2000}, {independent, 3000}}
            : std::vector<std::pair<CTransactionRef, CAmount>>{{child, 2000}, {parent, 1000}, {independent, 3000}};
        for (const auto& [tx, delta] : records) {
            dump << TX_WITH_WITNESS(*tx) << now << int64_t{delta};
        }
        dump << std::map<Txid, CAmount>{}
             << std::set<Txid>{parent->GetHash(), child->GetHash(), independent->GetHash()};
        std::ofstream file{path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    };
    const auto check_state = [&](bool child_present) {
        CTxMemPool& destination = *Assert(m_node.mempool);
        BOOST_REQUIRE_EQUAL(destination.size(), child_present ? 3U : 2U);
        BOOST_CHECK(destination.exists(parent->GetHash()));
        BOOST_CHECK(destination.exists(independent->GetHash()));
        BOOST_CHECK_EQUAL(destination.exists(child->GetHash()), child_present);
        if (child_present) {
            BOOST_CHECK_EQUAL(destination.info(child->GetHash()).nFeeDelta, 2000);
        }
        BOOST_CHECK_EQUAL(destination.info(parent->GetHash()).nFeeDelta, 1000);
        BOOST_CHECK_EQUAL(destination.info(independent->GetHash()).nFeeDelta, 3000);

        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 3U);
        for (const auto& delta : deltas) {
            if (delta.txid == parent->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 1000);
            } else if (delta.txid == child->GetHash()) {
                BOOST_CHECK_EQUAL(delta.in_mempool, child_present);
                BOOST_CHECK_EQUAL(delta.delta, 2000);
            } else if (delta.txid == independent->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 3000);
            } else {
                BOOST_FAIL("unexpected prioritization entry");
            }
        }
        const std::set<Txid> expected_unbroadcast = child_present
            ? std::set<Txid>{parent->GetHash(), child->GetHash(), independent->GetHash()}
            : std::set<Txid>{parent->GetHash(), independent->GetHash()};
        BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);
    };

    write_dump(child_first_path, false);
    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, child_first_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_state(false);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*parent), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*independent), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(parent->GetHash());
        destination.ClearPrioritisation(child->GetHash());
        destination.ClearPrioritisation(independent->GetHash());
    }
    BOOST_REQUIRE_EQUAL(destination.size(), 0U);
    BOOST_REQUIRE(destination.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(destination.GetUnbroadcastTxs().empty());

    write_dump(parent_first_path, true);
    BOOST_REQUIRE(node::LoadMempool(destination, parent_first_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_state(true);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*parent), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*independent), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(parent->GetHash());
        destination.ClearPrioritisation(child->GetHash());
        destination.ClearPrioritisation(independent->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_REQUIRE(fs::remove(child_first_path));
    BOOST_REQUIRE(fs::remove(parent_first_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1BranchingDependencyOrdering, TestChain100Setup)
{
    mineBlocks(1);

    const CKey parent_key = GenerateRandomKey();
    const CScript parent_destination = GetScriptForDestination(PKHash(parent_key.GetPubKey()));
    const CTransactionRef parent = MakeTransactionRef(CreateValidMempoolTransaction(
        std::vector<CTransactionRef>{m_coinbase_txns[0]},
        std::vector<COutPoint>{COutPoint{m_coinbase_txns[0]->GetHash(), 0}},
        0,
        std::vector<CKey>{coinbaseKey},
        std::vector<CTxOut>{{24 * COIN, parent_destination}, {24 * COIN, parent_destination}},
        false));

    const CKey child_one_key = GenerateRandomKey();
    const CKey child_two_key = GenerateRandomKey();
    const CTransactionRef child_one = MakeTransactionRef(CreateValidMempoolTransaction(
        parent, 0, 101, parent_key, GetScriptForDestination(PKHash(child_one_key.GetPubKey())), 23 * COIN, false));
    const CTransactionRef child_two = MakeTransactionRef(CreateValidMempoolTransaction(
        parent, 1, 101, parent_key, GetScriptForDestination(PKHash(child_two_key.GetPubKey())), 23 * COIN, false));
    const CTransactionRef independent = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));

    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    const fs::path child_first_path = m_path_root / "mempool-v1-branching-child-first.dat";
    const fs::path parent_first_path = m_path_root / "mempool-v1-branching-parent-first.dat";
    const auto write_dump = [&](const fs::path& path, bool parent_first) {
        DataStream dump;
        dump << uint64_t{1} << uint64_t{4};
        const std::vector<std::pair<CTransactionRef, CAmount>> records = parent_first
            ? std::vector<std::pair<CTransactionRef, CAmount>>{{parent, 1000}, {child_one, 2000}, {child_two, 3000}, {independent, 4000}}
            : std::vector<std::pair<CTransactionRef, CAmount>>{{child_one, 2000}, {independent, 4000}, {parent, 1000}, {child_two, 3000}};
        for (const auto& [tx, delta] : records) {
            dump << TX_WITH_WITNESS(*tx) << now << int64_t{delta};
        }
        dump << std::map<Txid, CAmount>{}
             << std::set<Txid>{parent->GetHash(), child_one->GetHash(), child_two->GetHash(), independent->GetHash()};
        std::ofstream file{path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    };
    const auto check_state = [&](bool child_one_present) {
        CTxMemPool& destination = *Assert(m_node.mempool);
        BOOST_REQUIRE_EQUAL(destination.size(), child_one_present ? 4U : 3U);
        BOOST_CHECK(destination.exists(parent->GetHash()));
        BOOST_CHECK(destination.exists(child_two->GetHash()));
        BOOST_CHECK(destination.exists(independent->GetHash()));
        BOOST_CHECK_EQUAL(destination.exists(child_one->GetHash()), child_one_present);
        BOOST_CHECK_EQUAL(destination.info(parent->GetHash()).nFeeDelta, 1000);
        BOOST_CHECK_EQUAL(destination.info(child_two->GetHash()).nFeeDelta, 3000);
        BOOST_CHECK_EQUAL(destination.info(independent->GetHash()).nFeeDelta, 4000);
        if (child_one_present) {
            BOOST_CHECK_EQUAL(destination.info(child_one->GetHash()).nFeeDelta, 2000);
        }

        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 4U);
        for (const auto& delta : deltas) {
            if (delta.txid == parent->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 1000);
            } else if (delta.txid == child_one->GetHash()) {
                BOOST_CHECK_EQUAL(delta.in_mempool, child_one_present);
                BOOST_CHECK_EQUAL(delta.delta, 2000);
            } else if (delta.txid == child_two->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 3000);
            } else if (delta.txid == independent->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 4000);
            } else {
                BOOST_FAIL("unexpected prioritization entry");
            }
        }
        const std::set<Txid> expected_unbroadcast = child_one_present
            ? std::set<Txid>{parent->GetHash(), child_one->GetHash(), child_two->GetHash(), independent->GetHash()}
            : std::set<Txid>{parent->GetHash(), child_two->GetHash(), independent->GetHash()};
        BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);
    };

    write_dump(child_first_path, false);
    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, child_first_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_state(false);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*parent), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*independent), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(parent->GetHash());
        destination.ClearPrioritisation(child_one->GetHash());
        destination.ClearPrioritisation(child_two->GetHash());
        destination.ClearPrioritisation(independent->GetHash());
    }
    BOOST_REQUIRE_EQUAL(destination.size(), 0U);
    BOOST_REQUIRE(destination.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(destination.GetUnbroadcastTxs().empty());

    write_dump(parent_first_path, true);
    BOOST_REQUIRE(node::LoadMempool(destination, parent_first_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_state(true);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*parent), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*independent), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(parent->GetHash());
        destination.ClearPrioritisation(child_one->GetHash());
        destination.ClearPrioritisation(child_two->GetHash());
        destination.ClearPrioritisation(independent->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_REQUIRE(fs::remove(child_first_path));
    BOOST_REQUIRE(fs::remove(parent_first_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolV1AllOrphansDependencyOrdering, TestChain100Setup)
{
    mineBlocks(1);

    const CKey parent_key = GenerateRandomKey();
    const CScript parent_destination = GetScriptForDestination(PKHash(parent_key.GetPubKey()));
    const CTransactionRef parent = MakeTransactionRef(CreateValidMempoolTransaction(
        std::vector<CTransactionRef>{m_coinbase_txns[0]},
        std::vector<COutPoint>{COutPoint{m_coinbase_txns[0]->GetHash(), 0}},
        0,
        std::vector<CKey>{coinbaseKey},
        std::vector<CTxOut>{{24 * COIN, parent_destination}, {24 * COIN, parent_destination}},
        false));

    const CKey child_one_key = GenerateRandomKey();
    const CKey child_two_key = GenerateRandomKey();
    const CTransactionRef child_one = MakeTransactionRef(CreateValidMempoolTransaction(
        parent, 0, 101, parent_key, GetScriptForDestination(PKHash(child_one_key.GetPubKey())), 23 * COIN, false));
    const CTransactionRef child_two = MakeTransactionRef(CreateValidMempoolTransaction(
        parent, 1, 101, parent_key, GetScriptForDestination(PKHash(child_two_key.GetPubKey())), 23 * COIN, false));
    const CTransactionRef independent = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns[1], 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));

    const int64_t now = TicksSinceEpoch<std::chrono::seconds>(NodeClock::now());
    const fs::path orphan_first_path = m_path_root / "mempool-v1-all-orphans.dat";
    const fs::path parent_first_path = m_path_root / "mempool-v1-all-orphans-parent-first.dat";
    const auto write_dump = [&](const fs::path& path, bool parent_first) {
        DataStream dump;
        dump << uint64_t{1} << uint64_t{4};
        const std::vector<std::pair<CTransactionRef, CAmount>> records = parent_first
            ? std::vector<std::pair<CTransactionRef, CAmount>>{{parent, 1000}, {child_one, 2000}, {child_two, 3000}, {independent, 4000}}
            : std::vector<std::pair<CTransactionRef, CAmount>>{{child_one, 2000}, {independent, 4000}, {child_two, 3000}, {parent, 1000}};
        for (const auto& [tx, delta] : records) {
            dump << TX_WITH_WITNESS(*tx) << now << int64_t{delta};
        }
        dump << std::map<Txid, CAmount>{}
             << std::set<Txid>{parent->GetHash(), child_one->GetHash(), child_two->GetHash(), independent->GetHash()};
        std::ofstream file{path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    };
    const auto check_state = [&](bool children_present) {
        CTxMemPool& destination = *Assert(m_node.mempool);
        BOOST_REQUIRE_EQUAL(destination.size(), children_present ? 4U : 2U);
        BOOST_CHECK(destination.exists(parent->GetHash()));
        BOOST_CHECK(destination.exists(independent->GetHash()));
        BOOST_CHECK_EQUAL(destination.exists(child_one->GetHash()), children_present);
        BOOST_CHECK_EQUAL(destination.exists(child_two->GetHash()), children_present);
        BOOST_CHECK_EQUAL(destination.info(parent->GetHash()).nFeeDelta, 1000);
        BOOST_CHECK_EQUAL(destination.info(independent->GetHash()).nFeeDelta, 4000);
        if (children_present) {
            BOOST_CHECK_EQUAL(destination.info(child_one->GetHash()).nFeeDelta, 2000);
            BOOST_CHECK_EQUAL(destination.info(child_two->GetHash()).nFeeDelta, 3000);
        }

        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 4U);
        for (const auto& delta : deltas) {
            if (delta.txid == parent->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 1000);
            } else if (delta.txid == child_one->GetHash()) {
                BOOST_CHECK_EQUAL(delta.in_mempool, children_present);
                BOOST_CHECK_EQUAL(delta.delta, 2000);
            } else if (delta.txid == child_two->GetHash()) {
                BOOST_CHECK_EQUAL(delta.in_mempool, children_present);
                BOOST_CHECK_EQUAL(delta.delta, 3000);
            } else if (delta.txid == independent->GetHash()) {
                BOOST_CHECK(delta.in_mempool);
                BOOST_CHECK_EQUAL(delta.delta, 4000);
            } else {
                BOOST_FAIL("unexpected prioritization entry");
            }
        }
        const std::set<Txid> expected_unbroadcast = children_present
            ? std::set<Txid>{parent->GetHash(), child_one->GetHash(), child_two->GetHash(), independent->GetHash()}
            : std::set<Txid>{parent->GetHash(), independent->GetHash()};
        BOOST_CHECK(destination.GetUnbroadcastTxs() == expected_unbroadcast);
    };

    write_dump(orphan_first_path, false);
    CTxMemPool& destination = *Assert(m_node.mempool);
    BOOST_REQUIRE(node::LoadMempool(destination, orphan_first_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_state(false);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*parent), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*independent), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(parent->GetHash());
        destination.ClearPrioritisation(child_one->GetHash());
        destination.ClearPrioritisation(child_two->GetHash());
        destination.ClearPrioritisation(independent->GetHash());
    }
    BOOST_REQUIRE_EQUAL(destination.size(), 0U);
    BOOST_REQUIRE(destination.GetPrioritisedTransactions().empty());
    BOOST_REQUIRE(destination.GetUnbroadcastTxs().empty());

    write_dump(parent_first_path, true);
    BOOST_REQUIRE(node::LoadMempool(destination, parent_first_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_state(true);

    {
        LOCK2(::cs_main, destination.cs);
        destination.removeRecursive(CTransaction(*parent), REMOVAL_REASON_DUMMY);
        destination.removeRecursive(CTransaction(*independent), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(parent->GetHash());
        destination.ClearPrioritisation(child_one->GetHash());
        destination.ClearPrioritisation(child_two->GetHash());
        destination.ClearPrioritisation(independent->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_REQUIRE(fs::remove(orphan_first_path));
    BOOST_REQUIRE(fs::remove(parent_first_path));
}

BOOST_FIXTURE_TEST_CASE(MempoolUnsupportedVersionPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_CHECK_EQUAL(deltas.size(), 1U);
        if (deltas.size() == 1) {
            BOOST_CHECK(deltas.front().in_mempool);
            BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
        }
    };

    const fs::path unsupported_path = m_path_root / "mempool-unsupported-version.dat";
    DataStream unsupported;
    unsupported << uint64_t{99};
    {
        std::ofstream file{unsupported_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(unsupported.data()), unsupported.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, unsupported_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    const fs::path truncated_v2_path = m_path_root / "mempool-truncated-v2-header.dat";
    DataStream truncated_v2;
    truncated_v2 << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    {
        std::ofstream file{truncated_v2_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(truncated_v2.data()), truncated_v2.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, truncated_v2_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    BOOST_REQUIRE(fs::remove(unsupported_path));
    BOOST_REQUIRE(fs::remove(truncated_v2_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolMalformedObfuscationKeyPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_CHECK_EQUAL(deltas.size(), 1U);
        if (deltas.size() == 1) {
            BOOST_CHECK(deltas.front().in_mempool);
            BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
        }
    };

    for (const size_t key_size : {Obfuscation::KEY_SIZE - 1, Obfuscation::KEY_SIZE + 1}) {
        const fs::path dump_path = key_size < Obfuscation::KEY_SIZE
            ? m_path_root / "mempool-v2-short-obfuscation-key.dat"
            : m_path_root / "mempool-v2-long-obfuscation-key.dat";
        DataStream dump;
        dump << uint64_t{2} << std::vector<std::byte>(key_size);
        {
            std::ofstream file{dump_path.std_path(), std::ios::binary};
            file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
            file.close();
            BOOST_REQUIRE(file.good());
        }
        BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
            .use_current_time = true,
            .apply_fee_delta_priority = true,
            .apply_unbroadcast_set = true,
        }));
        check_existing_state();
        BOOST_REQUIRE(fs::remove(dump_path));
    }

    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolExtremeTransactionCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_CHECK_EQUAL(deltas.size(), 1U);
        if (deltas.size() == 1) {
            BOOST_CHECK(deltas.front().in_mempool);
            BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
        }
    };

    const fs::path dump_path = m_path_root / "mempool-v2-extreme-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE)
         << std::numeric_limits<uint64_t>::max();
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolNonCanonicalTransactionCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const fs::path dump_path = m_path_root / "mempool-v2-noncanonical-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> noncanonical_count{std::byte{0xfd}, std::byte{0x01}, std::byte{0x00}};
    dump.write(noncanonical_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_CHECK_EQUAL(destination.size(), 1U);
    BOOST_CHECK(destination.exists(existing->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
    BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, 7000);

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolNonCanonicalFeeDeltaCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const fs::path dump_path = m_path_root / "mempool-v2-noncanonical-fee-delta-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> transaction_count{std::byte{0}};
    const std::vector<std::byte> noncanonical_map_count{std::byte{0xfd}, std::byte{0x00}, std::byte{0x00}};
    dump.write(transaction_count);
    dump.write(noncanonical_map_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_CHECK_EQUAL(destination.size(), 1U);
    BOOST_CHECK(destination.exists(existing->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
    BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, 7000);

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolExtremeFeeDeltaCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const fs::path dump_path = m_path_root / "mempool-v2-extreme-fee-delta-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> transaction_count{std::byte{0}};
    const std::vector<std::byte> extreme_map_count(9, std::byte{0xff});
    dump.write(transaction_count);
    dump.write(extreme_map_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    BOOST_CHECK_EQUAL(destination.size(), 1U);
    BOOST_CHECK(destination.exists(existing->GetHash()));
    BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
    BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
    const auto deltas = destination.GetPrioritisedTransactions();
    BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
    BOOST_CHECK(deltas.front().in_mempool);
    BOOST_CHECK_EQUAL(deltas.front().delta, 7000);

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolExtremeUnbroadcastCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
        BOOST_CHECK(deltas.front().in_mempool);
        BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
    };

    const fs::path dump_path = m_path_root / "mempool-v2-extreme-unbroadcast-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> transaction_count{std::byte{0}};
    const std::vector<std::byte> map_count{std::byte{0}};
    const std::vector<std::byte> extreme_set_count(9, std::byte{0xff});
    dump.write(transaction_count);
    dump.write(map_count);
    dump.write(extreme_set_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolNonCanonicalUnbroadcastCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
        BOOST_CHECK(deltas.front().in_mempool);
        BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
    };

    const fs::path dump_path = m_path_root / "mempool-v2-noncanonical-unbroadcast-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> transaction_count{std::byte{0}};
    const std::vector<std::byte> map_count{std::byte{0}};
    const std::vector<std::byte> noncanonical_set_count{std::byte{0xfd}, std::byte{0x00}, std::byte{0x00}};
    dump.write(transaction_count);
    dump.write(map_count);
    dump.write(noncanonical_set_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolTruncatedUnbroadcastCountPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
        BOOST_CHECK(deltas.front().in_mempool);
        BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
    };

    const fs::path dump_path = m_path_root / "mempool-v2-truncated-unbroadcast-count.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> transaction_count{std::byte{0}};
    const std::vector<std::byte> map_count{std::byte{0}};
    dump.write(transaction_count);
    dump.write(map_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_FIXTURE_TEST_CASE(MempoolTruncatedUnbroadcastCountPrefixPreservesState, TestChain100Setup)
{
    const CTransactionRef existing = MakeTransactionRef(CreateValidMempoolTransaction(
        m_coinbase_txns.front(), 0, 0, coinbaseKey, GetScriptForDestination(PKHash(coinbaseKey.GetPubKey())), 49 * COIN, false));
    CTxMemPool& destination = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    {
        LOCK2(::cs_main, destination.cs);
        TryAddToMempool(destination, entry.Fee(1000).Time(Now<NodeSeconds>()).FromTx(existing));
    }
    destination.PrioritiseTransaction(existing->GetHash(), 7000);
    destination.AddUnbroadcastTx(existing->GetHash());

    const auto check_existing_state = [&] {
        BOOST_CHECK_EQUAL(destination.size(), 1U);
        BOOST_CHECK(destination.exists(existing->GetHash()));
        BOOST_CHECK_EQUAL(destination.info(existing->GetHash()).nFeeDelta, 7000);
        BOOST_CHECK(destination.GetUnbroadcastTxs() == std::set<Txid>{existing->GetHash()});
        const auto deltas = destination.GetPrioritisedTransactions();
        BOOST_REQUIRE_EQUAL(deltas.size(), 1U);
        BOOST_CHECK(deltas.front().in_mempool);
        BOOST_CHECK_EQUAL(deltas.front().delta, 7000);
    };

    const fs::path dump_path = m_path_root / "mempool-v2-truncated-unbroadcast-count-prefix.dat";
    DataStream dump;
    dump << uint64_t{2} << std::vector<std::byte>(Obfuscation::KEY_SIZE);
    const std::vector<std::byte> transaction_count{std::byte{0}};
    const std::vector<std::byte> map_count{std::byte{0}};
    const std::vector<std::byte> truncated_set_count{std::byte{0xfd}};
    dump.write(transaction_count);
    dump.write(map_count);
    dump.write(truncated_set_count);
    {
        std::ofstream file{dump_path.std_path(), std::ios::binary};
        file.write(reinterpret_cast<const char*>(dump.data()), dump.size());
        file.close();
        BOOST_REQUIRE(file.good());
    }
    BOOST_REQUIRE(!node::LoadMempool(destination, dump_path, m_node.chainman->ActiveChainstate(), {
        .use_current_time = true,
        .apply_fee_delta_priority = true,
        .apply_unbroadcast_set = true,
    }));
    check_existing_state();

    BOOST_REQUIRE(fs::remove(dump_path));
    {
        LOCK2(::cs_main, destination.cs);
        destination.RemoveUnbroadcastTx(existing->GetHash());
        destination.removeRecursive(CTransaction(*existing), REMOVAL_REASON_DUMMY);
        destination.ClearPrioritisation(existing->GetHash());
    }
    BOOST_CHECK_EQUAL(destination.size(), 0U);
    BOOST_CHECK(destination.GetUnbroadcastTxs().empty());
    BOOST_CHECK(destination.GetPrioritisedTransactions().empty());
}

BOOST_AUTO_TEST_SUITE_END()
