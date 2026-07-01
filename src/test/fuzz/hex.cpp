// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <primitives/block.h>
#include <primitives/transaction_identifier.h>
#include <pubkey.h>
#include <rpc/protocol.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
CBlockHeader MakeNonNullBlockHeader()
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
    static_cast<CBlockHeader&>(block) = MakeNonNullBlockHeader();
    block.vtx.push_back(MakeTransactionRef(CMutableTransaction{}));
    block.fChecked = true;
    block.m_checked_witness_commitment = true;
    block.m_checked_merkle_root = true;
    return block;
}
} // namespace

FUZZ_TARGET(hex)
{
    const std::string random_hex_string(buffer.begin(), buffer.end());
    const std::vector<unsigned char> data = ParseHex(random_hex_string);
    const std::vector<std::byte> bytes{ParseHex<std::byte>(random_hex_string)};
    assert(std::ranges::equal(std::as_bytes(std::span{data}), bytes));
    const std::string hex_data = HexStr(data);
    if (const auto parsed_hex{TryParseHex<unsigned char>(random_hex_string)}) {
        std::string normalized_hex;
        normalized_hex.reserve(random_hex_string.size());
        for (const char c : random_hex_string) {
            if (!IsSpace(c)) {
                normalized_hex.push_back(c);
            }
        }
        if (!normalized_hex.empty()) {
            assert(IsHex(normalized_hex));
        }
        assert(normalized_hex.size() == parsed_hex->size() * 2);
        assert(HexStr(*parsed_hex) == ToLower(normalized_hex));
        assert(std::ranges::equal(data, *parsed_hex));
    }
    if (IsHex(random_hex_string)) {
        assert(ToLower(random_hex_string) == hex_data);
    }
    if (uint256::FromHex(random_hex_string)) {
        assert(random_hex_string.length() == 64);
        assert(Txid::FromHex(random_hex_string));
        assert(Wtxid::FromHex(random_hex_string));
        assert(uint256::FromUserHex(random_hex_string));
    }
    if (const auto result{uint256::FromUserHex(random_hex_string)}) {
        const auto result_string{result->ToString()}; // ToString() returns a fixed-length string without "0x" prefix
        assert(result_string.length() == 64);
        assert(IsHex(result_string));
        assert(TryParseHex(result_string));
        assert(Txid::FromHex(result_string));
        assert(Wtxid::FromHex(result_string));
        assert(uint256::FromHex(result_string));
    }
    try {
        (void)HexToPubKey(random_hex_string);
    } catch (const UniValue& e) {
        AssertJSONRPCError(e);
        assert(e["code"].getInt<int>() == RPC_INVALID_ADDRESS_OR_KEY);
    }
    CBlockHeader block_header{MakeNonNullBlockHeader()};
    if (!DecodeHexBlockHeader(block_header, random_hex_string)) {
        assert(block_header.IsNull());
        assert(block_header.nVersion == 0);
        assert(block_header.hashPrevBlock.IsNull());
        assert(block_header.hashMerkleRoot.IsNull());
        assert(block_header.nTime == 0);
        assert(block_header.nBits == 0);
        assert(block_header.nNonce == 0);
    }
    CBlock block{MakeNonNullBlock()};
    if (!DecodeHexBlk(block, random_hex_string)) {
        assert(block.IsNull());
        assert(block.vtx.empty());
        assert(!block.fChecked);
        assert(!block.m_checked_witness_commitment);
        assert(!block.m_checked_merkle_root);
    }
}
