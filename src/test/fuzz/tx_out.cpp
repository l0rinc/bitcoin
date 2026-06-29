// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace {
void AssertDustContracts(const CTxOut& tx_out, const CFeeRate& dust_relay_fee)
{
    const CAmount dust_threshold{GetDustThreshold(tx_out, dust_relay_fee)};
    const bool is_dust{IsDust(tx_out, dust_relay_fee)};
    assert(is_dust == (tx_out.nValue < dust_threshold));

    int witness_version;
    std::vector<unsigned char> witness_program;
    if (tx_out.scriptPubKey.IsUnspendable()) {
        assert(dust_threshold == 0);
    } else {
        const uint64_t spend_size{tx_out.scriptPubKey.IsWitnessProgram(witness_version, witness_program) ?
            uint64_t{32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4} :
            uint64_t{32 + 4 + 1 + 107 + 4}};
        assert(dust_threshold == dust_relay_fee.GetFee(GetSerializeSize(tx_out) + spend_size));
    }

    CMutableTransaction tx;
    tx.vout.push_back(tx_out);
    const std::vector<uint32_t> dust_outputs{GetDust(CTransaction{tx}, dust_relay_fee)};
    if (is_dust) {
        assert(dust_outputs == std::vector<uint32_t>{0});
    } else {
        assert(dust_outputs.empty());
    }

    CTxOut boundary_tx_out{tx_out};
    boundary_tx_out.nValue = dust_threshold;
    assert(!IsDust(boundary_tx_out, dust_relay_fee));
    if (dust_threshold > 0) {
        boundary_tx_out.nValue = dust_threshold - 1;
        assert(IsDust(boundary_tx_out, dust_relay_fee));
    }
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

    for (const CFeeRate& dust_relay_fee : std::array{
             CFeeRate{0},
             CFeeRate{1},
             CFeeRate{DUST_RELAY_TX_FEE},
             CFeeRate{3702},
         }) {
        AssertDustContracts(tx_out, dust_relay_fee);
    }
    (void)RecursiveDynamicUsage(tx_out);

    (void)tx_out.ToString();
    (void)tx_out.IsNull();
    tx_out.SetNull();
    assert(tx_out.IsNull());
}
