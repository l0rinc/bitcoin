// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <policy/policy.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/serfloat.h>
#include <util/time.h>
#include <validationinterface.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <limits>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(policyestimator_tests, ChainTestingSetup)

namespace {

struct FeeStatsValues {
    double feerate_avg{0};
    double tx_count{0};
    double conf_avg{0};
    double fail_avg{0};
};

struct TestEncodedDoubleFormatter {
    template <typename Stream>
    void Ser(Stream& s, double v)
    {
        s << EncodeDouble(v);
    }

    template <typename Stream>
    void Unser(Stream& s, double& v)
    {
        uint64_t encoded;
        s >> encoded;
        v = DecodeDouble(encoded);
    }
};

void WriteEncodedDoubleVector(AutoFile& file, const std::vector<double>& values)
{
    file << Using<VectorFormatter<TestEncodedDoubleFormatter>>(values);
}

void WriteEncodedDoubleMatrix(AutoFile& file, const std::vector<std::vector<double>>& values)
{
    file << Using<VectorFormatter<VectorFormatter<TestEncodedDoubleFormatter>>>(values);
}

void WriteFeeStats(AutoFile& file, const size_t num_buckets, const FeeStatsValues& values = {})
{
    file << EncodeDouble(0.5); // decay
    file << uint32_t{1};       // scale
    WriteEncodedDoubleVector(file, std::vector<double>(num_buckets, values.feerate_avg));
    WriteEncodedDoubleVector(file, std::vector<double>(num_buckets, values.tx_count));
    WriteEncodedDoubleMatrix(file, std::vector<std::vector<double>>{std::vector<double>(num_buckets, values.conf_avg)});
    WriteEncodedDoubleMatrix(file, std::vector<std::vector<double>>{std::vector<double>(num_buckets, values.fail_avg)});
}

int CurrentFeesFileVersion(const fs::path& path)
{
    fs::remove(path);
    CBlockPolicyEstimator fee_estimator{path, DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    {
        AutoFile out{fsbridge::fopen(path, "wb")};
        BOOST_REQUIRE(!out.IsNull());
        BOOST_REQUIRE(fee_estimator.Write(out));
        BOOST_REQUIRE_EQUAL(out.fclose(), 0);
    }

    int version;
    {
        AutoFile in{fsbridge::fopen(path, "rb")};
        BOOST_REQUIRE(!in.IsNull());
        in >> version;
        BOOST_REQUIRE_EQUAL(in.fclose(), 0);
    }
    fs::remove(path);
    return version;
}

void WriteEstimatorFile(const fs::path& path, int version, const std::vector<double>& buckets, const FeeStatsValues& values = {})
{
    fs::remove(path);
    AutoFile out{fsbridge::fopen(path, "wb")};
    BOOST_REQUIRE(!out.IsNull());
    out << version;
    out << uint32_t{0}; // nBestSeenHeight
    out << uint32_t{0}; // historicalFirst
    out << uint32_t{0}; // historicalBest
    WriteEncodedDoubleVector(out, buckets);
    WriteFeeStats(out, buckets.size(), values);
    WriteFeeStats(out, buckets.size());
    WriteFeeStats(out, buckets.size());
    BOOST_REQUIRE_EQUAL(out.fclose(), 0);
}

bool ReadEstimatorFile(const fs::path& path, const fs::path& estimator_path)
{
    fs::remove(estimator_path);
    CBlockPolicyEstimator fee_estimator{estimator_path, DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    AutoFile in{fsbridge::fopen(path, "rb")};
    BOOST_REQUIRE(!in.IsNull());
    const bool read{fee_estimator.Read(in)};
    BOOST_REQUIRE_EQUAL(in.fclose(), 0);
    fs::remove(estimator_path);
    return read;
}

} // namespace

BOOST_AUTO_TEST_CASE(BlockPolicyEstimates)
{
    CBlockPolicyEstimator feeEst{FeeestPath(*m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    CTxMemPool& mpool = *Assert(m_node.mempool);
    m_node.validation_signals->RegisterValidationInterface(&feeEst);
    TestMemPoolEntryHelper entry;
    CAmount basefee(2000);
    CAmount deltaFee(100);
    std::vector<CAmount> feeV;
    feeV.reserve(10);

    // Populate vectors of increasing fees
    for (int j = 0; j < 10; j++) {
        feeV.push_back(basefee * (j+1));
    }

    // Store the hashes of transactions that have been
    // added to the mempool by their associate fee
    // txHashes[j] is populated with transactions either of
    // fee = basefee * (j+1)
    std::vector<Txid> txHashes[10];

    // Create a transaction template
    CScript garbage;
    for (unsigned int i = 0; i < 128; i++)
        garbage.push_back('X');
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vin[0].scriptSig = garbage;
    tx.vout.resize(1);
    tx.vout[0].nValue=0LL;
    CFeeRate baseRate(basefee, GetVirtualTransactionSize(CTransaction(tx)));

    // Create a fake block
    std::vector<CTransactionRef> block;
    int blocknum = 0;

    // Loop through 200 blocks
    // At a decay .9952 and 4 fee transactions per block
    // This makes the tx count about 2.5 per bucket, well above the 0.1 threshold
    while (blocknum < 200) {
        for (int j = 0; j < 10; j++) { // For each fee
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k; // make transaction unique
                {
                    LOCK2(cs_main, mpool.cs);
                    TryAddToMempool(mpool, entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                    // Since TransactionAddedToMempool callbacks are generated in ATMP,
                    // not TryAddToMempool, we cheat and create one manually here
                    const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                    const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                      feeV[j],
                                                                                      virtual_size,
                                                                                      entry.nHeight,
                                                                                      /*mempool_limit_bypassed=*/false,
                                                                                      /*submitted_in_package=*/false,
                                                                                      /*chainstate_is_current=*/true,
                                                                                      /*has_no_mempool_parents=*/true)};
                    m_node.validation_signals->TransactionAddedToMempool(tx_info, mpool.GetAndIncrementSequence());
                }
                txHashes[j].push_back(tx.GetHash());
            }
        }
        //Create blocks where higher fee txs are included more often
        for (int h = 0; h <= blocknum%10; h++) {
            // 10/10 blocks add highest fee transactions
            // 9/10 blocks add 2nd highest and so on until ...
            // 1/10 blocks add lowest fee transactions
            while (txHashes[9-h].size()) {
                CTransactionRef ptx = mpool.get(txHashes[9-h].back());
                if (ptx)
                    block.push_back(ptx);
                txHashes[9-h].pop_back();
            }
        }

        {
            LOCK(mpool.cs);
            mpool.removeForBlock(block, ++blocknum);
        }

        block.clear();
        // Check after just a few txs that combining buckets works as expected
        if (blocknum == 3) {
            // Wait for fee estimator to catch up
            m_node.validation_signals->SyncWithValidationInterfaceQueue();
            // At this point we should need to combine 3 buckets to get enough data points
            // So estimateFee(1) should fail and estimateFee(2) should return somewhere around
            // 9*baserate.  estimateFee(2) %'s are 100,100,90 = average 97%
            BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
            BOOST_CHECK(feeEst.estimateFee(2).GetFeePerK() < 9*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(feeEst.estimateFee(2).GetFeePerK() > 9*baseRate.GetFeePerK() - deltaFee);
        }
    }

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    std::vector<CAmount> origFeeEst;
    // Highest feerate is 10*baseRate and gets in all blocks,
    // second highest feerate is 9*baseRate and gets in 9/10 blocks = 90%,
    // third highest feerate is 8*base rate, and gets in 8/10 blocks = 80%,
    // so estimateFee(1) would return 10*baseRate but is hardcoded to return failure
    // Second highest feerate has 100% chance of being included by 2 blocks,
    // so estimateFee(2) should return 9*baseRate etc...
    for (int i = 1; i < 10;i++) {
        origFeeEst.push_back(feeEst.estimateFee(i).GetFeePerK());
        if (i > 2) { // Fee estimates should be monotonically decreasing
            BOOST_CHECK(origFeeEst[i-1] <= origFeeEst[i-2]);
        }
        int mult = 11-i;
        if (i % 2 == 0) { //At scale 2, test logic is only correct for even targets
            BOOST_CHECK(origFeeEst[i-1] < mult*baseRate.GetFeePerK() + deltaFee);
            BOOST_CHECK(origFeeEst[i-1] > mult*baseRate.GetFeePerK() - deltaFee);
        }
    }
    // Fill out rest of the original estimates
    for (int i = 10; i <= 48; i++) {
        origFeeEst.push_back(feeEst.estimateFee(i).GetFeePerK());
    }

    // Mine 50 more blocks with no transactions happening, estimates shouldn't change
    // We haven't decayed the moving average enough so we still have enough data points in every bucket
    while (blocknum < 250) {
        LOCK(mpool.cs);
        mpool.removeForBlock(block, ++blocknum);
    }

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] + deltaFee);
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }


    // Mine 15 more blocks with lots of transactions happening and not getting mined
    // Estimates should go up
    while (blocknum < 265) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                {
                    LOCK2(cs_main, mpool.cs);
                    TryAddToMempool(mpool, entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                    // Since TransactionAddedToMempool callbacks are generated in ATMP,
                    // not TryAddToMempool, we cheat and create one manually here
                    const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                    const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                      feeV[j],
                                                                                      virtual_size,
                                                                                      entry.nHeight,
                                                                                      /*mempool_limit_bypassed=*/false,
                                                                                      /*submitted_in_package=*/false,
                                                                                      /*chainstate_is_current=*/true,
                                                                                      /*has_no_mempool_parents=*/true)};
                    m_node.validation_signals->TransactionAddedToMempool(tx_info, mpool.GetAndIncrementSequence());
                }
                txHashes[j].push_back(tx.GetHash());
            }
        }
        {
            LOCK(mpool.cs);
            mpool.removeForBlock(block, ++blocknum);
        }
    }

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    for (int i = 1; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine all those transactions
    // Estimates should still not be below original
    for (int j = 0; j < 10; j++) {
        while(txHashes[j].size()) {
            CTransactionRef ptx = mpool.get(txHashes[j].back());
            if (ptx)
                block.push_back(ptx);
            txHashes[j].pop_back();
        }
    }

    {
        LOCK(mpool.cs);
        mpool.removeForBlock(block, 266);
    }
    block.clear();

    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();

    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 10;i++) {
        BOOST_CHECK(feeEst.estimateFee(i) == CFeeRate(0) || feeEst.estimateFee(i).GetFeePerK() > origFeeEst[i-1] - deltaFee);
    }

    // Mine 400 more blocks where everything is mined every block
    // Estimates should be below original estimates
    while (blocknum < 665) {
        for (int j = 0; j < 10; j++) { // For each fee multiple
            for (int k = 0; k < 4; k++) { // add 4 fee txs
                tx.vin[0].prevout.n = 10000*blocknum+100*j+k;
                {
                    LOCK2(cs_main, mpool.cs);
                    TryAddToMempool(mpool, entry.Fee(feeV[j]).Time(Now<NodeSeconds>()).Height(blocknum).FromTx(tx));
                    // Since TransactionAddedToMempool callbacks are generated in ATMP,
                    // not TryAddToMempool, we cheat and create one manually here
                    const int64_t virtual_size = GetVirtualTransactionSize(*MakeTransactionRef(tx));
                    const NewMempoolTransactionInfo tx_info{NewMempoolTransactionInfo(MakeTransactionRef(tx),
                                                                                      feeV[j],
                                                                                      virtual_size,
                                                                                      entry.nHeight,
                                                                                      /*mempool_limit_bypassed=*/false,
                                                                                      /*submitted_in_package=*/false,
                                                                                      /*chainstate_is_current=*/true,
                                                                                      /*has_no_mempool_parents=*/true)};
                    m_node.validation_signals->TransactionAddedToMempool(tx_info, mpool.GetAndIncrementSequence());
                }
                CTransactionRef ptx = mpool.get(tx.GetHash());
                if (ptx)
                    block.push_back(ptx);

            }
        }

        {
            LOCK(mpool.cs);
            mpool.removeForBlock(block, ++blocknum);
        }

        block.clear();
    }
    // Wait for fee estimator to catch up
    m_node.validation_signals->SyncWithValidationInterfaceQueue();
    BOOST_CHECK(feeEst.estimateFee(1) == CFeeRate(0));
    for (int i = 2; i < 9; i++) { // At 9, the original estimate was already at the bottom (b/c scale = 2)
        BOOST_CHECK(feeEst.estimateFee(i).GetFeePerK() < origFeeEst[i-1] - deltaFee);
    }
}

BOOST_AUTO_TEST_CASE(reject_corrupt_fee_estimate_file_vectors)
{
    const fs::path version_path{m_args.GetDataDirBase() / "fee_estimator_version.dat"};
    const fs::path corrupt_path{m_args.GetDataDirBase() / "fee_estimator_corrupt.dat"};
    const fs::path estimator_path{m_args.GetDataDirBase() / "fee_estimator_unused.dat"};
    const int current_version{CurrentFeesFileVersion(version_path)};
    const std::vector<double> sane_buckets{100.0, 1e99};
    const double nan{std::numeric_limits<double>::quiet_NaN()};
    FeeStatsValues values;

    WriteEstimatorFile(corrupt_path, current_version, sane_buckets);
    BOOST_CHECK(ReadEstimatorFile(corrupt_path, estimator_path));

    WriteEstimatorFile(corrupt_path, current_version, {100.0, nan});
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    WriteEstimatorFile(corrupt_path, current_version, {1000.0, 100.0});
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    WriteEstimatorFile(corrupt_path, current_version, {100.0, 1000.0});
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values.feerate_avg = nan;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.feerate_avg = -1.0;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.tx_count = nan;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.tx_count = -1.0;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.conf_avg = nan;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.conf_avg = -1.0;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.fail_avg = nan;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    values = {};
    values.fail_avg = -1.0;
    WriteEstimatorFile(corrupt_path, current_version, sane_buckets, values);
    BOOST_CHECK(!ReadEstimatorFile(corrupt_path, estimator_path));

    fs::remove(corrupt_path);
}

BOOST_AUTO_TEST_CASE(estimate_raw_fee_rejects_invalid_threshold)
{
    const fs::path version_path{m_args.GetDataDirBase() / "fee_estimator_version.dat"};
    const fs::path estimator_data_path{m_args.GetDataDirBase() / "fee_estimator_threshold.dat"};
    const fs::path estimator_path{m_args.GetDataDirBase() / "fee_estimator_unused.dat"};
    const int current_version{CurrentFeesFileVersion(version_path)};

    FeeStatsValues values;
    values.feerate_avg = 1000;
    values.tx_count = 1;
    values.conf_avg = 1;
    WriteEstimatorFile(estimator_data_path, current_version, {100.0, 1e99}, values);

    fs::remove(estimator_path);
    CBlockPolicyEstimator fee_estimator{estimator_path, DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    {
        AutoFile in{fsbridge::fopen(estimator_data_path, "rb")};
        BOOST_REQUIRE(!in.IsNull());
        BOOST_REQUIRE(fee_estimator.Read(in));
        BOOST_REQUIRE_EQUAL(in.fclose(), 0);
    }
    BOOST_REQUIRE_GT(fee_estimator.estimateRawFee(1, 0.0, FeeEstimateHorizon::MED_HALFLIFE).GetFeePerK(), 0);

    const auto check_invalid_threshold{[&](const double threshold) {
        EstimationResult result;
        result.pass.start = 17;
        result.fail.start = 19;
        result.decay = 23;
        result.scale = 29;
        BOOST_CHECK(fee_estimator.estimateRawFee(1, threshold, FeeEstimateHorizon::MED_HALFLIFE, &result) == CFeeRate(0));
        BOOST_CHECK_EQUAL(result.pass.start, 17);
        BOOST_CHECK_EQUAL(result.fail.start, 19);
        BOOST_CHECK_EQUAL(result.decay, 23);
        BOOST_CHECK_EQUAL(result.scale, 29U);
    }};

    check_invalid_threshold(-std::numeric_limits<double>::infinity());
    check_invalid_threshold(std::numeric_limits<double>::quiet_NaN());
    check_invalid_threshold(-1.0);
    check_invalid_threshold(1.0 + std::numeric_limits<double>::epsilon());
    check_invalid_threshold(std::numeric_limits<double>::infinity());

    fs::remove(estimator_data_path);
    fs::remove(estimator_path);
}

BOOST_AUTO_TEST_SUITE_END()
