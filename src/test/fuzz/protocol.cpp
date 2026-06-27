// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

FUZZ_TARGET(protocol)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CInv> inv = ConsumeDeserializable<CInv>(fuzzed_data_provider);
    if (!inv) {
        return;
    }
    assert(inv->IsMsgTx() == (inv->type == MSG_TX));
    assert(inv->IsMsgBlk() == (inv->type == MSG_BLOCK));
    assert(inv->IsMsgWtx() == (inv->type == MSG_WTX));
    assert(inv->IsMsgFilteredBlk() == (inv->type == MSG_FILTERED_BLOCK));
    assert(inv->IsMsgCmpctBlk() == (inv->type == MSG_CMPCT_BLOCK));
    assert(inv->IsMsgWitnessBlk() == (inv->type == MSG_WITNESS_BLOCK));
    assert(inv->IsGenTxMsg() == (inv->type == MSG_TX || inv->type == MSG_WTX || inv->type == MSG_WITNESS_TX));
    assert(inv->IsGenBlkMsg() == (inv->type == MSG_BLOCK || inv->type == MSG_FILTERED_BLOCK || inv->type == MSG_CMPCT_BLOCK || inv->type == MSG_WITNESS_BLOCK));

    if (inv->IsGenTxMsg()) {
        const GenTxid gtxid{ToGenTxid(*inv)};
        assert(gtxid.ToUint256() == inv->hash);
        assert(gtxid.IsWtxid() == inv->IsMsgWtx());
    }

    const auto expected_message_type{[&]() -> std::optional<std::string> {
        std::string prefix;
        if (inv->type & MSG_WITNESS_FLAG) prefix = "witness-";
        switch (inv->type & MSG_TYPE_MASK) {
        case MSG_TX: return prefix + NetMsgType::TX;
        case MSG_WTX: return prefix + "wtx";
        case MSG_BLOCK: return prefix + NetMsgType::BLOCK;
        case MSG_FILTERED_BLOCK: return prefix + NetMsgType::MERKLEBLOCK;
        case MSG_CMPCT_BLOCK: return prefix + NetMsgType::CMPCTBLOCK;
        default: return std::nullopt;
        }
    }()};
    try {
        const std::string message_type{inv->GetMessageType()};
        assert(expected_message_type);
        assert(message_type == *expected_message_type);
        assert(inv->ToString().starts_with(message_type + " "));
    } catch (const std::out_of_range&) {
        assert(!expected_message_type);
        assert(inv->ToString().find(inv->hash.ToString()) != std::string::npos);
    }
    (void)inv->ToString();
    const std::optional<CInv> another_inv = ConsumeDeserializable<CInv>(fuzzed_data_provider);
    if (!another_inv) {
        return;
    }
    assert(!(*inv < *inv));
    if (*inv < *another_inv) {
        assert(!(*another_inv < *inv));
    }
    (void)(*inv < *another_inv);
}
