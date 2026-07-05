// Copyright (c) 2012-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <merkleblock.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <set>
#include <vector>

BOOST_AUTO_TEST_SUITE(merkleblock_tests)

BOOST_AUTO_TEST_CASE(merkleblock_rejects_invalid_block_tx_refs)
{
    test_only_CheckFailuresAreExceptionsNotAborts failed_asserts_throw{};
    std::set<Txid> txids;

    BOOST_CHECK_THROW(CMerkleBlock(CBlock{}, txids), NonFatalCheckError);

    CBloomFilter filter{10, 0.000001, 0, BLOOM_UPDATE_NONE};
    CBlock resized_block;
    resized_block.vtx.resize(1);
    BOOST_CHECK_THROW(CMerkleBlock(resized_block, filter), NonFatalCheckError);
}

BOOST_AUTO_TEST_CASE(merkleblock_bit_byte_roundtrip_padding)
{
    const auto check_roundtrip = [](const std::vector<bool>& bits) {
        const std::vector<unsigned char> bytes{BitsToBytes(bits)};
        const std::vector<bool> decoded{BytesToBits(bytes)};

        BOOST_CHECK_EQUAL(bytes.size(), (bits.size() + 7) / 8);
        BOOST_CHECK_EQUAL(decoded.size(), bytes.size() * 8);
        BOOST_CHECK(decoded.size() >= bits.size());
        BOOST_CHECK(std::equal(bits.begin(), bits.end(), decoded.begin()));
        BOOST_CHECK(std::all_of(decoded.begin() + bits.size(), decoded.end(),
            [](bool bit) { return !bit; }));
        BOOST_CHECK(BitsToBytes(decoded) == bytes);
    };

    check_roundtrip({});
    check_roundtrip({true});
    check_roundtrip({false, true, false, true, true});
    check_roundtrip({true, false, true, false, true, false, true, false});
    check_roundtrip({true, true, false, false, true, false, false, true, true});
}

/**
 * Create a CMerkleBlock using a list of txids which will be found in the
 * given block.
 */
BOOST_AUTO_TEST_CASE(merkleblock_construct_from_txids_found)
{
    CBlock block = getBlock13b8a();

    std::set<Txid> txids;

    // Last txn in block.
    constexpr Txid txhash1{"74d681e0e03bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20"};

    // Second txn in block.
    constexpr Txid txhash2{"f9fc751cb7dc372406a9f8d738d5e6f8f63bab71986a39cf36ee70ee17036d07"};

    txids.insert(txhash1);
    txids.insert(txhash2);

    CMerkleBlock merkleBlock(block, txids);

    BOOST_CHECK_EQUAL(merkleBlock.header.GetHash().GetHex(), block.GetHash().GetHex());

    // vMatchedTxn is only used when bloom filter is specified.
    BOOST_CHECK_EQUAL(merkleBlock.vMatchedTxn.size(), 0U);

    std::vector<Txid> vMatched;
    std::vector<unsigned int> vIndex;

    BOOST_CHECK_EQUAL(merkleBlock.txn.ExtractMatches(vMatched, vIndex).GetHex(), block.hashMerkleRoot.GetHex());
    BOOST_CHECK_EQUAL(vMatched.size(), 2U);

    // Ordered by occurrence in depth-first tree traversal.
    BOOST_CHECK_EQUAL(vMatched[0], txhash2);
    BOOST_CHECK_EQUAL(vIndex[0], 1U);

    BOOST_CHECK_EQUAL(vMatched[1], txhash1);
    BOOST_CHECK_EQUAL(vIndex[1], 8U);
}


/**
 * Create a CMerkleBlock using a list of txids which will not be found in the
 * given block.
 */
BOOST_AUTO_TEST_CASE(merkleblock_construct_from_txids_not_found)
{
    CBlock block = getBlock13b8a();

    std::set<Txid> txids2;
    txids2.insert(Txid{"c0ffee00003bafa802c8aa084379aa98d9fcd632ddc2ed9782b586ec87451f20"});
    CMerkleBlock merkleBlock(block, txids2);

    BOOST_CHECK_EQUAL(merkleBlock.header.GetHash().GetHex(), block.GetHash().GetHex());
    BOOST_CHECK_EQUAL(merkleBlock.vMatchedTxn.size(), 0U);

    std::vector<Txid> vMatched;
    std::vector<unsigned int> vIndex;

    BOOST_CHECK_EQUAL(merkleBlock.txn.ExtractMatches(vMatched, vIndex).GetHex(), block.hashMerkleRoot.GetHex());
    BOOST_CHECK_EQUAL(vMatched.size(), 0U);
    BOOST_CHECK_EQUAL(vIndex.size(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()
