// Copyright (c) 2011-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/common.h>
#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <policy/policy.h>
#include <test/util/txmempool.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/serfloat.h>
#include <util/time.h>
#include <validationinterface.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

uint16_t ReadLE16(const std::vector<unsigned char>& data, size_t& position)
{
    const uint16_t value{static_cast<uint16_t>(static_cast<uint16_t>(data[position]) |
                                              static_cast<uint16_t>(data[position + 1]) << 8)};
    position += 2;
    return value;
}

uint32_t ReadLE32(const std::vector<unsigned char>& data, size_t& position)
{
    uint32_t value{0};
    for (unsigned int i = 0; i < 4; ++i) value |= uint32_t{data[position++]} << (8 * i);
    return value;
}

uint64_t ReadLE64(const std::vector<unsigned char>& data, size_t& position)
{
    uint64_t value{0};
    for (unsigned int i = 0; i < 8; ++i) value |= uint64_t{data[position++]} << (8 * i);
    return value;
}

uint64_t ReadCompactSize(const std::vector<unsigned char>& data, size_t& position)
{
    const unsigned char marker{data[position++]};
    if (marker < 253) return marker;
    if (marker == 253) return ReadLE16(data, position);
    if (marker == 254) return ReadLE32(data, position);
    return ReadLE64(data, position);
}

size_t FindBucketPosition(const std::vector<unsigned char>& data, size_t& bucket_count)
{
    size_t position{16}; // version, best height, and historical range
    bucket_count = ReadCompactSize(data, position);
    if (bucket_count <= 1 || position + 8 * bucket_count > data.size()) {
        throw std::runtime_error{"unexpected fee-estimator bucket vector"};
    }
    return position;
}

void WriteBucket(std::vector<unsigned char>& data, size_t position, double value)
{
    std::array<unsigned char, sizeof(uint64_t)> encoded{};
    WriteLE64(encoded.data(), EncodeDouble(value));
    std::copy(encoded.begin(), encoded.end(), data.begin() + position);
}

enum class BucketMutation {
    NONFINITE,
    NONINCREASING,
    MISSING_SENTINEL,
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(policyestimator_tests, ChainTestingSetup)

BOOST_AUTO_TEST_CASE(RejectInvalidPersistedBuckets)
{
    const fs::path path{m_node.args->GetDataDirNet() / "goal98-fee-buckets.dat"};
    CBlockPolicyEstimator writer{path, DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    const std::array<std::pair<const char*, BucketMutation>, 3> mutations{{
        {"non-finite", BucketMutation::NONFINITE},
        {"non-increasing", BucketMutation::NONINCREASING},
        {"missing high sentinel", BucketMutation::MISSING_SENTINEL},
    }};

    for (const auto& [name, mutation] : mutations) {
        {
            AutoFile output{fsbridge::fopen(path, "wb")};
            BOOST_REQUIRE(!output.IsNull());
            BOOST_REQUIRE(writer.Write(output));
            BOOST_REQUIRE_EQUAL(output.fclose(), 0);
        }

        std::ifstream input{path.utf8string(), std::ios::binary};
        std::vector<unsigned char> data{std::istreambuf_iterator<char>{input}, {}};
        BOOST_REQUIRE(input.good() || input.eof());
        size_t bucket_count{0};
        const size_t buckets{FindBucketPosition(data, bucket_count)};

        switch (mutation) {
        case BucketMutation::NONFINITE:
            WriteBucket(data, buckets, std::numeric_limits<double>::quiet_NaN());
            break;
        case BucketMutation::NONINCREASING:
            std::copy_n(data.begin() + buckets, 8, data.begin() + buckets + 8);
            break;
        case BucketMutation::MISSING_SENTINEL:
            WriteBucket(data, buckets + 8 * (bucket_count - 1), 0.0);
            break;
        }

        std::ofstream output{path.utf8string(), std::ios::binary | std::ios::trunc};
        output.write(reinterpret_cast<const char*>(data.data()), data.size());
        output.close();
        BOOST_REQUIRE(output.good());

        CBlockPolicyEstimator reader{fs::path{}, DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
        AutoFile mutated{fsbridge::fopen(path, "rb")};
        BOOST_REQUIRE(!mutated.IsNull());
        BOOST_CHECK_MESSAGE(!reader.Read(mutated), "accepted " << name << " persisted bucket vector");
        BOOST_REQUIRE_EQUAL(mutated.fclose(), 0);
    }
}

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

BOOST_AUTO_TEST_SUITE_END()
