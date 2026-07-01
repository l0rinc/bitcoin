// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(block_tests)

namespace {
void CheckNullHeader(const CBlockHeader& header)
{
    BOOST_CHECK(header.IsNull());
    BOOST_CHECK_EQUAL(header.nVersion, 0);
    BOOST_CHECK(header.hashPrevBlock.IsNull());
    BOOST_CHECK(header.hashMerkleRoot.IsNull());
    BOOST_CHECK_EQUAL(header.nTime, 0U);
    BOOST_CHECK_EQUAL(header.nBits, 0U);
    BOOST_CHECK_EQUAL(header.nNonce, 0U);
}

void CheckNullBlock(const CBlock& block)
{
    CheckNullHeader(block);
    BOOST_CHECK(block.vtx.empty());
    BOOST_CHECK(!block.fChecked);
    BOOST_CHECK(!block.m_checked_witness_commitment);
    BOOST_CHECK(!block.m_checked_merkle_root);
}

CBlockHeader MakeNonNullHeader()
{
    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256::ONE;
    header.hashMerkleRoot = uint256{2};
    header.nTime = 3;
    header.nBits = 4;
    header.nNonce = 5;
    return header;
}

CBlock MakeNonNullBlock()
{
    CBlock block;
    static_cast<CBlockHeader&>(block) = MakeNonNullHeader();
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
    block.fChecked = true;
    block.m_checked_witness_commitment = true;
    block.m_checked_merkle_root = true;
    return block;
}
} // namespace

BOOST_AUTO_TEST_CASE(block_setnull_clears_all_fields)
{
    CBlockHeader header;
    header.nVersion = 1;
    header.hashPrevBlock = uint256::ONE;
    header.hashMerkleRoot = uint256{2};
    header.nTime = 3;
    header.nBits = 4;
    header.nNonce = 5;

    header.SetNull();
    CheckNullHeader(header);

    CBlock block;
    block.nVersion = 6;
    block.hashPrevBlock = uint256{7};
    block.hashMerkleRoot = uint256{8};
    block.nTime = 9;
    block.nBits = 10;
    block.nNonce = 11;
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
    block.fChecked = true;
    block.m_checked_witness_commitment = true;
    block.m_checked_merkle_root = true;

    block.SetNull();
    CheckNullBlock(block);
}

BOOST_AUTO_TEST_CASE(decode_hex_block_failure_clears_output)
{
    CBlockHeader header{MakeNonNullHeader()};
    BOOST_CHECK(!DecodeHexBlockHeader(header, "not hex"));
    CheckNullHeader(header);

    header = MakeNonNullHeader();
    BOOST_CHECK(!DecodeHexBlockHeader(header, "00"));
    CheckNullHeader(header);

    CBlock block{MakeNonNullBlock()};
    BOOST_CHECK(!DecodeHexBlk(block, "not hex"));
    CheckNullBlock(block);

    block = MakeNonNullBlock();
    BOOST_CHECK(!DecodeHexBlk(block, "00"));
    CheckNullBlock(block);
}

BOOST_AUTO_TEST_SUITE_END()
