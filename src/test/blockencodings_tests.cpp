// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <pow.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/txmempool.h>
#include <util/check.h>

#include <test/util/common.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <limits>

const std::vector<std::pair<Wtxid, CTransactionRef>> empty_extra_txn;

BOOST_FIXTURE_TEST_SUITE(blockencodings_tests, RegTestingSetup)

static CMutableTransaction BuildTransactionTestCase() {
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig.resize(10);
    tx.vout.resize(1);
    tx.vout[0].nValue = 42;
    return tx;
}

static CBlock BuildBlockTestCase(FastRandomContext& ctx) {
    CBlock block;
    CMutableTransaction tx = BuildTransactionTestCase();

    block.vtx.resize(3);
    block.vtx[0] = MakeTransactionRef(tx);
    block.nVersion = 42;
    block.hashPrevBlock = ctx.rand256();
    block.nBits = 0x207fffff;

    tx.vin[0].prevout.hash = Txid::FromUint256(ctx.rand256());
    tx.vin[0].prevout.n = 0;
    block.vtx[1] = MakeTransactionRef(tx);

    tx.vin.resize(10);
    for (size_t i = 0; i < tx.vin.size(); i++) {
        tx.vin[i].prevout.hash = Txid::FromUint256(ctx.rand256());
        tx.vin[i].prevout.n = 0;
    }
    block.vtx[2] = MakeTransactionRef(tx);

    bool mutated;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    assert(!mutated);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, Params().GetConsensus())) ++block.nNonce;
    return block;
}

// Number of shared use_counts we expect for a tx we haven't touched
// (block + mempool entry + our copy from the GetSharedTx call)
constexpr long SHARED_TX_OFFSET{3};

class TestPartiallyDownloadedBlock : public PartiallyDownloadedBlock
{
public:
    using PartiallyDownloadedBlock::PartiallyDownloadedBlock;

    size_t AvailableTxCount() const
    {
        return std::count_if(txn_available.begin(), txn_available.end(), [](const auto& tx) { return tx != nullptr; });
    }

    size_t PrefilledCount() const { return prefilled_count; }
    size_t MempoolCount() const { return mempool_count; }
    size_t ExtraCount() const { return extra_count; }
};

class TestCBlockHeaderAndShortTxIDs : public CBlockHeaderAndShortTxIDs
{
public:
    using CBlockHeaderAndShortTxIDs::CBlockHeaderAndShortTxIDs;

    void ResizePrefilledTx(size_t size)
    {
        prefilledtxn.resize(size);
    }
};

BOOST_AUTO_TEST_CASE(SimpleRoundTripTest)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    LOCK2(cs_main, pool.cs);
    TryAddToMempool(pool, entry.FromTx(block.vtx[2]));
    BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 0);

    // Do a simple ShortTxIDs RT
    {
        CBlockHeaderAndShortTxIDs shortIDs{block, rand_ctx.rand64()};

        DataStream stream{};
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, empty_extra_txn) == READ_STATUS_OK);
        BOOST_CHECK( partialBlock.IsTxAvailable(0));
        BOOST_CHECK(!partialBlock.IsTxAvailable(1));
        BOOST_CHECK( partialBlock.IsTxAvailable(2));

        BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 1);

        size_t poolSize = pool.size();
        pool.removeRecursive(*block.vtx[2], MemPoolRemovalReason::REPLACED);
        BOOST_CHECK_EQUAL(pool.size(), poolSize - 1);

        CBlock block2;
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            BOOST_CHECK(partialBlock.FillBlock(block2, {}, /*segwit_active=*/true) == READ_STATUS_INVALID); // No transactions
            partialBlock = tmp;
        }

        // Too many transactions
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            BOOST_CHECK(tmp.FillBlock(block2, {block.vtx[1], block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_INVALID);
            BOOST_CHECK(tmp.header.IsNull());
            BOOST_CHECK(tmp.FillBlock(block2, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_INVALID);
        }

        // Mutation failure
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            tmp.m_check_block_mutated_mock = [](const CBlock&, bool) { return true; };
            BOOST_CHECK(tmp.FillBlock(block2, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_FAILED);
            BOOST_CHECK(tmp.header.IsNull());
            BOOST_CHECK(tmp.FillBlock(block2, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_INVALID);
        }

        // Wrong transaction
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            partialBlock.FillBlock(block2, {block.vtx[2]}, /*segwit_active=*/true); // Current implementation doesn't check txn here, but don't require that
            partialBlock = tmp;
        }
        bool mutated;
        BOOST_CHECK(block.hashMerkleRoot != BlockMerkleRoot(block2, &mutated));

        CBlock block3;
        BOOST_CHECK(partialBlock.FillBlock(block3, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block3.GetHash().ToString());
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block3, &mutated).ToString());
        BOOST_CHECK(!mutated);
    }
}

BOOST_AUTO_TEST_CASE(HeaderAndShortIDsRejectsInvalidBlockTxRefs)
{
    test_only_CheckFailuresAreExceptionsNotAborts failed_asserts_throw{};
    auto rand_ctx(FastRandomContext(uint256{42}));

    CBlock empty_block;
    BOOST_CHECK_THROW(CBlockHeaderAndShortTxIDs(empty_block, rand_ctx.rand64()), NonFatalCheckError);

    CBlock resized_block;
    resized_block.vtx.resize(1);
    BOOST_CHECK_THROW(CBlockHeaderAndShortTxIDs(resized_block, rand_ctx.rand64()), NonFatalCheckError);

    resized_block.vtx[0] = MakeTransactionRef(BuildTransactionTestCase());
    resized_block.vtx.resize(2);
    BOOST_CHECK_THROW(CBlockHeaderAndShortTxIDs(resized_block, rand_ctx.rand64()), NonFatalCheckError);
}

class TestHeaderAndShortIDs {
    // Utility to encode custom CBlockHeaderAndShortTxIDs
public:
    CBlockHeader header;
    uint64_t nonce;
    std::vector<uint64_t> shorttxids;
    std::vector<PrefilledTransaction> prefilledtxn;

    explicit TestHeaderAndShortIDs(const CBlockHeaderAndShortTxIDs& orig) {
        DataStream stream{};
        stream << orig;
        stream >> *this;
    }
    explicit TestHeaderAndShortIDs(const CBlock& block, FastRandomContext& ctx) :
        TestHeaderAndShortIDs(CBlockHeaderAndShortTxIDs{block, ctx.rand64()}) {}

    uint64_t GetShortID(const Wtxid& txhash) const {
        DataStream stream{};
        stream << *this;
        CBlockHeaderAndShortTxIDs base;
        stream >> base;
        return base.GetShortID(txhash);
    }

    SERIALIZE_METHODS(TestHeaderAndShortIDs, obj) { READWRITE(obj.header, obj.nonce, Using<VectorFormatter<CustomUintFormatter<CBlockHeaderAndShortTxIDs::SHORTTXIDS_LENGTH>>>(obj.shorttxids), obj.prefilledtxn); }
};

BOOST_AUTO_TEST_CASE(InitDataFailureResetsPartialBlock)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    TestHeaderAndShortIDs invalid_ids(block, rand_ctx);
    CMutableTransaction null_tx;
    invalid_ids.prefilledtxn.push_back({0, MakeTransactionRef(std::move(null_tx))});

    DataStream invalid_stream{};
    invalid_stream << invalid_ids;

    CBlockHeaderAndShortTxIDs decoded_invalid;
    invalid_stream >> decoded_invalid;

    PartiallyDownloadedBlock partial_block(&pool);
    BOOST_CHECK(partial_block.InitData(decoded_invalid, empty_extra_txn) == READ_STATUS_INVALID);
    BOOST_CHECK(partial_block.header.IsNull());

    CBlock rejected_block;
    BOOST_CHECK(partial_block.FillBlock(rejected_block, {}, /*segwit_active=*/true) == READ_STATUS_INVALID);

    CBlockHeaderAndShortTxIDs valid_ids{block, rand_ctx.rand64()};
    BOOST_CHECK(partial_block.InitData(valid_ids, empty_extra_txn) == READ_STATUS_OK);
    BOOST_CHECK(partial_block.IsTxAvailable(0));

    TestHeaderAndShortIDs overflow_ids(block, rand_ctx);
    overflow_ids.prefilledtxn.push_back({std::numeric_limits<uint16_t>::max(), block.vtx[0]});

    DataStream overflow_stream{};
    overflow_stream << overflow_ids;

    CBlockHeaderAndShortTxIDs decoded_overflow;
    overflow_stream >> decoded_overflow;

    PartiallyDownloadedBlock overflow_block(&pool);
    BOOST_CHECK(overflow_block.InitData(decoded_overflow, empty_extra_txn) == READ_STATUS_INVALID);
    BOOST_CHECK(overflow_block.header.IsNull());

    TestCBlockHeaderAndShortTxIDs null_prefilled_ids{block, rand_ctx.rand64()};
    null_prefilled_ids.ResizePrefilledTx(2);

    PartiallyDownloadedBlock null_prefilled_block(&pool);
    BOOST_CHECK(null_prefilled_block.InitData(null_prefilled_ids, empty_extra_txn) == READ_STATUS_INVALID);
    BOOST_CHECK(null_prefilled_block.header.IsNull());
}

BOOST_AUTO_TEST_CASE(FillBlockResetsPartialBlock)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    {
        LOCK2(cs_main, pool.cs);
        TryAddToMempool(pool, entry.FromTx(block.vtx[2]));
    }

    CBlockHeaderAndShortTxIDs short_ids{block, rand_ctx.rand64()};
    TestPartiallyDownloadedBlock partial_block(&pool);
    BOOST_CHECK(partial_block.InitData(short_ids, empty_extra_txn) == READ_STATUS_OK);
    BOOST_CHECK_EQUAL(partial_block.PrefilledCount(), 1U);
    BOOST_CHECK_EQUAL(partial_block.MempoolCount(), 1U);
    BOOST_CHECK_EQUAL(partial_block.ExtraCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.AvailableTxCount(), 2U);

    CBlock reconstructed_block;
    BOOST_CHECK(partial_block.FillBlock(reconstructed_block, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_OK);
    BOOST_CHECK(partial_block.header.IsNull());
    BOOST_CHECK_EQUAL(partial_block.AvailableTxCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.PrefilledCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.MempoolCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.ExtraCount(), 0U);

    BOOST_CHECK(partial_block.InitData(short_ids, empty_extra_txn) == READ_STATUS_OK);
    BOOST_CHECK(partial_block.IsTxAvailable(0));
    BOOST_CHECK(!partial_block.IsTxAvailable(1));
    BOOST_CHECK(partial_block.IsTxAvailable(2));

    CBlock reconstructed_again;
    BOOST_CHECK(partial_block.FillBlock(reconstructed_again, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_OK);
    BOOST_CHECK_EQUAL(block.GetHash().ToString(), reconstructed_again.GetHash().ToString());
}

BOOST_AUTO_TEST_CASE(FillBlockTooFewTransactionsClearsPartialBlock)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    {
        LOCK2(cs_main, pool.cs);
        TryAddToMempool(pool, entry.FromTx(block.vtx[2]));
    }

    CBlockHeaderAndShortTxIDs short_ids{block, rand_ctx.rand64()};
    TestPartiallyDownloadedBlock partial_block(&pool);
    BOOST_CHECK(partial_block.InitData(short_ids, empty_extra_txn) == READ_STATUS_OK);
    BOOST_CHECK(partial_block.IsTxAvailable(0));
    BOOST_CHECK(!partial_block.IsTxAvailable(1));
    BOOST_CHECK(partial_block.IsTxAvailable(2));

    CBlock rejected_block;
    BOOST_CHECK(partial_block.FillBlock(rejected_block, {}, /*segwit_active=*/true) == READ_STATUS_INVALID);
    BOOST_CHECK(partial_block.header.IsNull());
    BOOST_CHECK_EQUAL(partial_block.AvailableTxCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.PrefilledCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.MempoolCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.ExtraCount(), 0U);

    CBlock retry_block;
    BOOST_CHECK(partial_block.FillBlock(retry_block, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_INVALID);

    BOOST_CHECK(partial_block.InitData(short_ids, empty_extra_txn) == READ_STATUS_OK);
    CBlock reconstructed_block;
    BOOST_CHECK(partial_block.FillBlock(reconstructed_block, {block.vtx[1]}, /*segwit_active=*/true) == READ_STATUS_OK);
    BOOST_CHECK_EQUAL(block.GetHash().ToString(), reconstructed_block.GetHash().ToString());
}

BOOST_AUTO_TEST_CASE(NonCoinbasePreforwardRTTest)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    LOCK2(cs_main, pool.cs);
    TryAddToMempool(pool, entry.FromTx(block.vtx[2]));
    BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 0);

    Txid txhash;

    // Test with pre-forwarding tx 1, but not coinbase
    {
        TestHeaderAndShortIDs shortIDs(block, rand_ctx);
        shortIDs.prefilledtxn.resize(1);
        shortIDs.prefilledtxn[0] = {1, block.vtx[1]};
        shortIDs.shorttxids.resize(2);
        shortIDs.shorttxids[0] = shortIDs.GetShortID(block.vtx[0]->GetWitnessHash());
        shortIDs.shorttxids[1] = shortIDs.GetShortID(block.vtx[2]->GetWitnessHash());

        DataStream stream{};
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, empty_extra_txn) == READ_STATUS_OK);
        BOOST_CHECK(!partialBlock.IsTxAvailable(0));
        BOOST_CHECK( partialBlock.IsTxAvailable(1));
        BOOST_CHECK( partialBlock.IsTxAvailable(2));

        BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 1); // +1 because of partialBlock

        CBlock block2;
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            BOOST_CHECK(partialBlock.FillBlock(block2, {}, /*segwit_active=*/true) == READ_STATUS_INVALID); // No transactions
            partialBlock = tmp;
        }

        // Wrong transaction
        {
            PartiallyDownloadedBlock tmp = partialBlock;
            partialBlock.FillBlock(block2, {block.vtx[1]}, /*segwit_active=*/true); // Current implementation doesn't check txn here, but don't require that
            partialBlock = tmp;
        }
        BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 2); // +2 because of partialBlock and block2
        bool mutated;
        BOOST_CHECK(block.hashMerkleRoot != BlockMerkleRoot(block2, &mutated));

        CBlock block3;
        PartiallyDownloadedBlock partialBlockCopy = partialBlock;
        BOOST_CHECK(partialBlock.FillBlock(block3, {block.vtx[0]}, /*segwit_active=*/true) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block3.GetHash().ToString());
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block3, &mutated).ToString());
        BOOST_CHECK(!mutated);

        BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 3); // +2 because of partialBlock and block2 and block3

        txhash = block.vtx[2]->GetHash();
        block.vtx.clear();
        block2.vtx.clear();
        block3.vtx.clear();
        BOOST_CHECK_EQUAL(pool.get(txhash).use_count(), SHARED_TX_OFFSET + 1 - 1); // + 1 because of partialBlock; -1 because of block.
    }
    BOOST_CHECK_EQUAL(pool.get(txhash).use_count(), SHARED_TX_OFFSET - 1); // -1 because of block
}

BOOST_AUTO_TEST_CASE(SufficientPreforwardRTTest)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    LOCK2(cs_main, pool.cs);
    TryAddToMempool(pool, entry.FromTx(block.vtx[1]));
    BOOST_CHECK_EQUAL(pool.get(block.vtx[1]->GetHash()).use_count(), SHARED_TX_OFFSET + 0);

    Txid txhash;

    // Test with pre-forwarding coinbase + tx 2 with tx 1 in mempool
    {
        TestHeaderAndShortIDs shortIDs(block, rand_ctx);
        shortIDs.prefilledtxn.resize(2);
        shortIDs.prefilledtxn[0] = {0, block.vtx[0]};
        shortIDs.prefilledtxn[1] = {1, block.vtx[2]}; // id == 1 as it is 1 after index 1
        shortIDs.shorttxids.resize(1);
        shortIDs.shorttxids[0] = shortIDs.GetShortID(block.vtx[1]->GetWitnessHash());

        DataStream stream{};
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, empty_extra_txn) == READ_STATUS_OK);
        BOOST_CHECK( partialBlock.IsTxAvailable(0));
        BOOST_CHECK( partialBlock.IsTxAvailable(1));
        BOOST_CHECK( partialBlock.IsTxAvailable(2));

        BOOST_CHECK_EQUAL(pool.get(block.vtx[1]->GetHash()).use_count(), SHARED_TX_OFFSET + 1);

        CBlock block2;
        PartiallyDownloadedBlock partialBlockCopy = partialBlock;
        BOOST_CHECK(partialBlock.FillBlock(block2, {}, /*segwit_active=*/true) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block2.GetHash().ToString());
        bool mutated;
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block2, &mutated).ToString());
        BOOST_CHECK(!mutated);

        txhash = block.vtx[1]->GetHash();
        block.vtx.clear();
        block2.vtx.clear();
        BOOST_CHECK_EQUAL(pool.get(txhash).use_count(), SHARED_TX_OFFSET + 1 - 1); // + 1 because of partialBlock; -1 because of block.
    }
    BOOST_CHECK_EQUAL(pool.get(txhash).use_count(), SHARED_TX_OFFSET - 1); // -1 because of block
}

BOOST_AUTO_TEST_CASE(EmptyBlockRoundTripTest)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    CMutableTransaction coinbase = BuildTransactionTestCase();

    CBlock block;
    auto rand_ctx(FastRandomContext(uint256{42}));
    block.vtx.resize(1);
    block.vtx[0] = MakeTransactionRef(std::move(coinbase));
    block.nVersion = 42;
    block.hashPrevBlock = rand_ctx.rand256();
    block.nBits = 0x207fffff;

    bool mutated;
    block.hashMerkleRoot = BlockMerkleRoot(block, &mutated);
    assert(!mutated);
    while (!CheckProofOfWork(block.GetHash(), block.nBits, Params().GetConsensus())) ++block.nNonce;

    // Test simple header round-trip with only coinbase
    {
        CBlockHeaderAndShortTxIDs shortIDs{block, rand_ctx.rand64()};

        DataStream stream{};
        stream << shortIDs;

        CBlockHeaderAndShortTxIDs shortIDs2;
        stream >> shortIDs2;

        PartiallyDownloadedBlock partialBlock(&pool);
        BOOST_CHECK(partialBlock.InitData(shortIDs2, empty_extra_txn) == READ_STATUS_OK);
        BOOST_CHECK(partialBlock.IsTxAvailable(0));

        CBlock block2;
        std::vector<CTransactionRef> vtx_missing;
        BOOST_CHECK(partialBlock.FillBlock(block2, vtx_missing, /*segwit_active=*/true) == READ_STATUS_OK);
        BOOST_CHECK_EQUAL(block.GetHash().ToString(), block2.GetHash().ToString());
        BOOST_CHECK_EQUAL(block.hashMerkleRoot.ToString(), BlockMerkleRoot(block2, &mutated).ToString());
        BOOST_CHECK(!mutated);
    }
}

BOOST_AUTO_TEST_CASE(ReceiveWithExtraTransactions) {
    CTxMemPool& pool = *Assert(m_node.mempool);
    TestMemPoolEntryHelper entry;
    auto rand_ctx(FastRandomContext(uint256{42}));

    CMutableTransaction mtx = BuildTransactionTestCase();
    mtx.vin[0].prevout.hash = Txid::FromUint256(rand_ctx.rand256());
    mtx.vin[0].prevout.n = 0;
    const CTransactionRef non_block_tx = MakeTransactionRef(std::move(mtx));

    CBlock block(BuildBlockTestCase(rand_ctx));
    std::vector<std::pair<Wtxid, CTransactionRef>> extra_txn;
    extra_txn.resize(10);

    LOCK2(cs_main, pool.cs);
    TryAddToMempool(pool, entry.FromTx(block.vtx[2]));
    BOOST_CHECK_EQUAL(pool.get(block.vtx[2]->GetHash()).use_count(), SHARED_TX_OFFSET + 0);
    // Ensure the non_block_tx is actually not in the block
    for (const auto &block_tx : block.vtx) {
        BOOST_CHECK_NE(block_tx->GetHash(), non_block_tx->GetHash());
    }
    // Ensure block.vtx[1] is not in pool
    BOOST_CHECK_EQUAL(pool.get(block.vtx[1]->GetHash()), nullptr);

    {
        const CBlockHeaderAndShortTxIDs cmpctblock{block, rand_ctx.rand64()};
        PartiallyDownloadedBlock partial_block(&pool);
        PartiallyDownloadedBlock partial_block_with_extra(&pool);

        BOOST_CHECK(partial_block.InitData(cmpctblock, extra_txn) == READ_STATUS_OK);
        BOOST_CHECK( partial_block.IsTxAvailable(0));
        BOOST_CHECK(!partial_block.IsTxAvailable(1));
        BOOST_CHECK( partial_block.IsTxAvailable(2));

        // Add an unrelated tx to extra_txn:
        extra_txn[0] = {non_block_tx->GetWitnessHash(), non_block_tx};
        // and a tx from the block that's not in the mempool:
        extra_txn[1] = {block.vtx[1]->GetWitnessHash(), block.vtx[1]};

        BOOST_CHECK(partial_block_with_extra.InitData(cmpctblock, extra_txn) == READ_STATUS_OK);
        BOOST_CHECK(partial_block_with_extra.IsTxAvailable(0));
        // This transaction is now available via extra_txn:
        BOOST_CHECK(partial_block_with_extra.IsTxAvailable(1));
        BOOST_CHECK(partial_block_with_extra.IsTxAvailable(2));
    }
}

BOOST_AUTO_TEST_CASE(EmptyExtraTransactionsDoNotSatisfyShortIds)
{
    CTxMemPool& pool = *Assert(m_node.mempool);
    auto rand_ctx(FastRandomContext(uint256{42}));
    CBlock block(BuildBlockTestCase(rand_ctx));

    TestHeaderAndShortIDs short_ids(block, rand_ctx);
    const Wtxid empty_wtxid{Wtxid::FromUint256(uint256::ZERO)};
    short_ids.shorttxids[0] = short_ids.GetShortID(empty_wtxid);

    DataStream stream{};
    stream << short_ids;

    CBlockHeaderAndShortTxIDs decoded_short_ids;
    stream >> decoded_short_ids;

    const std::vector<std::pair<Wtxid, CTransactionRef>> extra_txn{{empty_wtxid, CTransactionRef{}}};

    TestPartiallyDownloadedBlock partial_block(&pool);
    BOOST_CHECK(partial_block.InitData(decoded_short_ids, extra_txn) == READ_STATUS_OK);
    BOOST_CHECK(partial_block.IsTxAvailable(0));
    BOOST_CHECK(!partial_block.IsTxAvailable(1));
    BOOST_CHECK(!partial_block.IsTxAvailable(2));
    BOOST_CHECK_EQUAL(partial_block.AvailableTxCount(), 1U);
    BOOST_CHECK_EQUAL(partial_block.PrefilledCount(), 1U);
    BOOST_CHECK_EQUAL(partial_block.MempoolCount(), 0U);
    BOOST_CHECK_EQUAL(partial_block.ExtraCount(), 0U);

    CBlock reconstructed_block;
    BOOST_CHECK(partial_block.FillBlock(reconstructed_block, {block.vtx[1], block.vtx[2]}, /*segwit_active=*/true) == READ_STATUS_OK);
    BOOST_CHECK_EQUAL(block.GetHash().ToString(), reconstructed_block.GetHash().ToString());
}

BOOST_AUTO_TEST_CASE(TransactionsRequestSerializationTest) {
    BlockTransactionsRequest req1;
    req1.blockhash = m_rng.rand256();
    req1.indexes.resize(4);
    req1.indexes[0] = 0;
    req1.indexes[1] = 1;
    req1.indexes[2] = 3;
    req1.indexes[3] = 4;

    DataStream stream{};
    stream << req1;

    BlockTransactionsRequest req2;
    stream >> req2;

    BOOST_CHECK_EQUAL(req1.blockhash.ToString(), req2.blockhash.ToString());
    BOOST_CHECK_EQUAL(req1.indexes.size(), req2.indexes.size());
    BOOST_CHECK_EQUAL(req1.indexes[0], req2.indexes[0]);
    BOOST_CHECK_EQUAL(req1.indexes[1], req2.indexes[1]);
    BOOST_CHECK_EQUAL(req1.indexes[2], req2.indexes[2]);
    BOOST_CHECK_EQUAL(req1.indexes[3], req2.indexes[3]);
}

BOOST_AUTO_TEST_CASE(TransactionsRequestSerializationRejectsNonIncreasingIndexes)
{
    BlockTransactionsRequest valid;
    valid.blockhash = m_rng.rand256();
    valid.indexes = {0, 1, 0xffff};
    DataStream valid_stream{};
    valid_stream << valid;
    BlockTransactionsRequest valid_roundtrip;
    valid_stream >> valid_roundtrip;
    BOOST_CHECK(valid_roundtrip.indexes == valid.indexes);

    BlockTransactionsRequest duplicate;
    duplicate.blockhash = m_rng.rand256();
    duplicate.indexes = {0, 1, 1};
    DataStream duplicate_stream{};
    BOOST_CHECK_THROW(duplicate_stream << duplicate, std::ios_base::failure);

    BlockTransactionsRequest decreasing;
    decreasing.blockhash = m_rng.rand256();
    decreasing.indexes = {2, 1};
    DataStream decreasing_stream{};
    BOOST_CHECK_THROW(decreasing_stream << decreasing, std::ios_base::failure);
}

BOOST_AUTO_TEST_CASE(TransactionsRequestDeserializationMaxTest) {
    // Check that the highest legal index is decoded correctly
    BlockTransactionsRequest req0;
    req0.blockhash = m_rng.rand256();
    req0.indexes.resize(1);
    req0.indexes[0] = 0xffff;
    DataStream stream{};
    stream << req0;

    BlockTransactionsRequest req1;
    stream >> req1;
    BOOST_CHECK_EQUAL(req0.indexes.size(), req1.indexes.size());
    BOOST_CHECK_EQUAL(req0.indexes[0], req1.indexes[0]);
}

BOOST_AUTO_TEST_CASE(TransactionsRequestDeserializationOverflowTest) {
    // Any set of index deltas that starts with N values that sum to (0x10000 - N)
    // causes the edge-case overflow that was originally not checked for. Such
    // a request cannot be created by serializing a real BlockTransactionsRequest
    // due to the overflow, so here we'll serialize from raw deltas.
    BlockTransactionsRequest req0;
    req0.blockhash = m_rng.rand256();
    req0.indexes.resize(3);
    req0.indexes[0] = 0x7000;
    req0.indexes[1] = 0x10000 - 0x7000 - 2;
    req0.indexes[2] = 0;
    DataStream stream{};
    stream << req0.blockhash;
    WriteCompactSize(stream, req0.indexes.size());
    WriteCompactSize(stream, req0.indexes[0]);
    WriteCompactSize(stream, req0.indexes[1]);
    WriteCompactSize(stream, req0.indexes[2]);

    BlockTransactionsRequest req1;
    try {
        stream >> req1;
        // before patch: deserialize above succeeds and this check fails, demonstrating the overflow
        BOOST_CHECK(req1.indexes[1] < req1.indexes[2]);
        // this shouldn't be reachable before or after patch
        BOOST_CHECK(0);
    } catch(std::ios_base::failure &) {
        // deserialize should fail
        BOOST_CHECK(true); // Needed to suppress "Test case [...] did not check any assertions"
    }
}

BOOST_AUTO_TEST_SUITE_END()
