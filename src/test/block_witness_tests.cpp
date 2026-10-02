// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/block_witness.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <cstdint>
#include <ios>
#include <memory>
#include <span>
#include <vector>

using namespace util::hex_literals;

BOOST_FIXTURE_TEST_SUITE(block_witness_tests, BasicTestingSetup)

static std::vector<std::byte> SerializeBlock(const CBlock& block, bool witness)
{
    DataStream stream;
    stream << (witness ? TX_WITH_WITNESS : TX_NO_WITNESS)(block);
    return {stream.begin(), stream.end()};
}

static void CheckStripped(const CBlock& block)
{
    const auto expected{SerializeBlock(block, false)};
    auto data{SerializeBlock(block, true)};
    data.insert(data.end(), {std::byte{0xff}, std::byte{0x00}}); // Trailing bytes are ignored by the decoder and excluded by the strip
    CBlock decoded;
    SpanReader{data} >> TX_WITH_WITNESS(decoded);
    BOOST_CHECK(SerializeBlock(decoded, false) == expected);
    SpanReader{expected} >> TX_NO_WITNESS(decoded);
    BOOST_CHECK(SerializeBlock(decoded, false) == expected);
    data.resize(StripWitness(data));
    BOOST_CHECK(data == expected);
    BOOST_CHECK_EQUAL(StripWitness(data), data.size());
    BOOST_CHECK(data == expected);
}

BOOST_AUTO_TEST_CASE(strip_mixed_transactions)
{
    CBlock block;
    block.nVersion = -1;
    block.nTime = 12345;
    block.nNonce = 54321;
    CheckStripped(block);

    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.emplace_back(50 * COIN, CScript{} << OP_TRUE);
    tx.vin[0].scriptWitness.stack = {std::vector<unsigned char>(32, 0)};
    tx.vout.emplace_back(0, CScript{} << OP_RETURN << "aa21a9ed0000000000000000000000000000000000000000000000000000000000000000"_hex_v_u8);
    block.vtx.push_back(MakeTransactionRef(tx));
    tx.vin[0].scriptWitness.SetNull();
    tx.nLockTime = 42;
    block.vtx.push_back(MakeTransactionRef(tx));
    tx.vin.resize(3);
    tx.vin[0].scriptWitness.stack = {{}, {1, 2, 3}};
    tx.vin[2].scriptWitness.stack = {{}}; // A nonempty stack containing an empty item is still witness data
    block.vtx.push_back(MakeTransactionRef(tx));
    block.vtx.push_back(block.vtx[1]);
    CheckStripped(block);

    // Preserve the decoder's empty-transaction encoding even though consensus rejects these transactions.
    CBlock empty_tx_block;
    empty_tx_block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
    CheckStripped(empty_tx_block);
}

BOOST_AUTO_TEST_CASE(strip_compactsize_lengths)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    CBlock block;
    for (const auto size : {0, 1, 252, 253, 65535, 65536}) {
        tx.vin[0].scriptSig.assign(size, 0x51);
        tx.vout[0].scriptPubKey = tx.vin[0].scriptSig;
        tx.vin[0].scriptWitness.stack = {std::vector<unsigned char>(size, 0x42)};
        block.vtx = {MakeTransactionRef(tx)};
        CheckStripped(block);
    }
    tx.vin[0].scriptSig.clear();
    tx.vout[0].scriptPubKey.clear();
    tx.vin[0].scriptWitness.stack = {{0x42}};
    const auto small_tx{MakeTransactionRef(tx)};
    for (const auto count : {1, 252, 253}) {
        tx.vin.resize(count);
        tx.vout.resize(count);
        tx.vin[0].scriptWitness.stack = std::vector<std::vector<unsigned char>>(count);
        block.vtx = {MakeTransactionRef(tx)};
        block.vtx.resize(count, small_tx);
        CheckStripped(block);
    }
}

BOOST_AUTO_TEST_CASE(strip_invalid_encodings)
{
    CMutableTransaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].scriptWitness.stack = {{1, 2, 3}};
    CBlock block;
    block.vtx = {MakeTransactionRef(tx), MakeTransactionRef(tx)};
    const auto check_rejected{[](std::vector<std::byte> data, const char* reason) {
        CBlock decoded;
        BOOST_CHECK_EXCEPTION(SpanReader{data} >> TX_WITH_WITNESS(decoded), std::ios_base::failure, HasReason(reason));
        BOOST_CHECK_EXCEPTION((void)StripWitness(data), std::ios_base::failure, HasReason(reason));
    }};
    const auto full{SerializeBlock(block, true)};
    for (size_t size{0}; size < full.size(); ++size) check_rejected({full.begin(), full.begin() + size}, "end of data");

    block.vtx.resize(1);
    const auto valid{SerializeBlock(block, true)};
    for (uint8_t flag : {2, 3, 0xff}) {
        auto data{valid};
        data[86] = std::byte{flag};
        check_rejected(data, "Unknown transaction optional data");
    }
    auto empty_witness{valid};
    empty_witness[139] = std::byte{0};
    check_rejected(empty_witness, "Superfluous witness record");

    // CompactSizes for transactions, inputs, scriptSig, outputs, scriptPubKey, witness items and item length
    for (const auto offset : {80, 87, 124, 129, 138, 139, 140}) {
        for (const auto& encoding : {"fd0000"_hex_v, "fe00000000"_hex_v, "ff0000000000000000"_hex_v, "fe01000002"_hex_v}) {
            auto data{valid};
            data.erase(data.begin() + offset);
            data.insert(data.begin() + offset, encoding.begin(), encoding.end());
            check_rejected(data, encoding == "fe01000002"_hex_v ? "size too large" : "non-canonical");
            for (size_t size{1}; size < encoding.size(); ++size) check_rejected({data.begin(), data.begin() + offset + size}, "end of data");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
