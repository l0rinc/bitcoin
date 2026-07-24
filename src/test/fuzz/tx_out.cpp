// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

#include <cstdint>
#include <vector>

namespace {

CAmount ExpectedDustThreshold(const CTxOut& tx_out, const CFeeRate& dust_relay_fee)
{
    if (tx_out.scriptPubKey.IsUnspendable()) return 0;

    int64_t spend_size{static_cast<int64_t>(GetSerializeSize(tx_out))};
    int witness_version{0};
    std::vector<unsigned char> witness_program;
    if (tx_out.scriptPubKey.IsWitnessProgram(witness_version, witness_program)) {
        spend_size += 32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4;
    } else {
        spend_size += 32 + 4 + 1 + 107 + 4;
    }
    return dust_relay_fee.GetFee(static_cast<int32_t>(spend_size));
}

} // namespace

FUZZ_TARGET(tx_out)
{
    CTxOut tx_out;
    try {
        SpanReader{buffer} >> tx_out;
    } catch (const std::ios_base::failure&) {
        return;
    }

    const CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
    const CTxOut before_policy{tx_out};
    const CAmount expected_dust{ExpectedDustThreshold(tx_out, dust_relay_fee)};
    assert(GetDustThreshold(tx_out, dust_relay_fee) == expected_dust);
    assert(IsDust(tx_out, dust_relay_fee) == (tx_out.nValue < expected_dust));
    assert(tx_out == before_policy);

    DataStream serialized;
    serialized << tx_out;
    assert(serialized.size() == ::GetSerializeSize(tx_out));
    CTxOut round_tripped;
    serialized >> round_tripped;
    assert(serialized.empty());
    assert(round_tripped == tx_out);

    (void)RecursiveDynamicUsage(tx_out);

    (void)tx_out.ToString();
    (void)tx_out.IsNull();
    tx_out.SetNull();
    assert(tx_out.IsNull());
    assert(tx_out.scriptPubKey.empty());
}
