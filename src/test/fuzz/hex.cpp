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

FUZZ_TARGET(hex)
{
    const std::string random_hex_string(buffer.begin(), buffer.end());
    const std::vector<unsigned char> data = ParseHex(random_hex_string);
    const std::vector<std::byte> bytes{ParseHex<std::byte>(random_hex_string)};
    assert(std::ranges::equal(std::as_bytes(std::span{data}), bytes));
    const std::string hex_data = HexStr(data);
    assert(hex_data.size() == data.size() * 2);
    const auto reparsed_hex_data{TryParseHex<uint8_t>(hex_data)};
    assert(reparsed_hex_data && std::ranges::equal(*reparsed_hex_data, data));
    std::string spaced_hex;
    for (size_t i{0}; i < hex_data.size(); i += 2) {
        if (!spaced_hex.empty()) {
            spaced_hex += ' ';
        }
        spaced_hex.append(hex_data, i, 2);
    }
    const auto wrapped_spaced_hex{" \t\n" + spaced_hex + " \r\n"};
    const auto parsed_spaced_hex{ParseHex(wrapped_spaced_hex)};
    assert(std::ranges::equal(parsed_spaced_hex, data));
    const auto reparsed_spaced_hex{TryParseHex<uint8_t>(wrapped_spaced_hex)};
    assert(reparsed_spaced_hex && std::ranges::equal(*reparsed_spaced_hex, data));
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
    } catch (const UniValue&) {
    }
    CBlockHeader block_header;
    (void)DecodeHexBlockHeader(block_header, random_hex_string);
    CBlock block;
    (void)DecodeHexBlk(block, random_hex_string);
}
