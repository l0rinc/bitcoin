// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <tinyformat.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::optional<std::string> ExpectedMessageType(const CInv& inv)
{
    const std::string witness_prefix = inv.type & MSG_WITNESS_FLAG ? "witness-" : "";
    switch (inv.type & MSG_TYPE_MASK) {
    case MSG_TX: return witness_prefix + NetMsgType::TX;
    case MSG_WTX: return witness_prefix + "wtx";
    case MSG_BLOCK: return witness_prefix + NetMsgType::BLOCK;
    case MSG_FILTERED_BLOCK: return witness_prefix + NetMsgType::MERKLEBLOCK;
    case MSG_CMPCT_BLOCK: return witness_prefix + NetMsgType::CMPCTBLOCK;
    default: return std::nullopt;
    }
}

void AssertCInvContracts(const CInv& inv)
{
    const std::optional<std::string> expected_message_type = ExpectedMessageType(inv);
    if (expected_message_type) {
        assert(inv.GetMessageType() == *expected_message_type);
    } else {
        bool threw{false};
        try {
            (void)inv.GetMessageType();
        } catch (const std::out_of_range&) {
            threw = true;
        }
        assert(threw);
    }

    const std::string expected_type = expected_message_type ? *expected_message_type : strprintf("0x%08x", inv.type);
    assert(inv.ToString() == expected_type + " " + inv.hash.ToString());

    assert(inv.IsMsgTx() == (inv.type == MSG_TX));
    assert(inv.IsMsgBlk() == (inv.type == MSG_BLOCK));
    assert(inv.IsMsgWtx() == (inv.type == MSG_WTX));
    assert(inv.IsMsgFilteredBlk() == (inv.type == MSG_FILTERED_BLOCK));
    assert(inv.IsMsgCmpctBlk() == (inv.type == MSG_CMPCT_BLOCK));
    assert(inv.IsMsgWitnessBlk() == (inv.type == MSG_WITNESS_BLOCK));
    assert(inv.IsGenTxMsg() == (inv.type == MSG_TX || inv.type == MSG_WTX || inv.type == MSG_WITNESS_TX));
    assert(inv.IsGenBlkMsg() == (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK || inv.type == MSG_CMPCT_BLOCK || inv.type == MSG_WITNESS_BLOCK));
}
} // namespace

FUZZ_TARGET(protocol)
{
    FuzzedDataProvider generated_provider(buffer.data(), buffer.size());
    const CInv generated_inv{generated_provider.ConsumeIntegral<uint32_t>(), ConsumeUInt256(generated_provider)};
    AssertCInvContracts(generated_inv);

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CInv> inv = ConsumeDeserializable<CInv>(fuzzed_data_provider);
    if (!inv) {
        return;
    }
    AssertCInvContracts(*inv);
    const std::optional<CInv> another_inv = ConsumeDeserializable<CInv>(fuzzed_data_provider);
    if (!another_inv) {
        return;
    }
    const bool expected_less = inv->type < another_inv->type ||
                               (inv->type == another_inv->type && inv->hash < another_inv->hash);
    assert((*inv < *another_inv) == expected_less);
    assert(!(*inv < *inv));
    assert(!(*another_inv < *another_inv));
    if (expected_less) {
        assert(!(*another_inv < *inv));
    }
}
