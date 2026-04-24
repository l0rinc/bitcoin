// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <primitives/block.h>
#include <primitives/transaction_identifier.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <test/fuzz/fuzz.h>
#include <uint256.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <test/util/check.h>

FUZZ_TARGET(hex)
{
    const std::string random_hex_string(buffer.begin(), buffer.end());
    const std::vector<unsigned char> data = ParseHex(random_hex_string);
    const std::vector<std::byte> bytes{ParseHex<std::byte>(random_hex_string)};
    CHECK(std::ranges::equal(std::as_bytes(std::span{data}), bytes));
    const std::string hex_data = HexStr(data);
    if (IsHex(random_hex_string)) {
        CHECK(ToLower(random_hex_string) == hex_data);
    }
    if (uint256::FromHex(random_hex_string)) {
        CHECK(random_hex_string.length() == 64);
        CHECK(Txid::FromHex(random_hex_string));
        CHECK(Wtxid::FromHex(random_hex_string));
        CHECK(uint256::FromUserHex(random_hex_string));
    }
    if (const auto result{uint256::FromUserHex(random_hex_string)}) {
        const auto result_string{result->ToString()}; // ToString() returns a fixed-length string without "0x" prefix
        CHECK(result_string.length() == 64);
        CHECK(IsHex(result_string));
        CHECK(TryParseHex(result_string));
        CHECK(Txid::FromHex(result_string));
        CHECK(Wtxid::FromHex(result_string));
        CHECK(uint256::FromHex(result_string));
    }
    try {
        (void)HexToPubKey(random_hex_string);
    } catch (const UniValue&) {
    }
    CBlockHeader block_header;
    (void)DecodeHexBlockHeader(block_header, random_hex_string);
    CBlock block;
    (void)DecodeHexBlk(block, random_hex_string);
}
