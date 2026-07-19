// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>

#include <chainparams.h>
#include <coins.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <core_memusage.h>
#include <hash.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/random.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/rbf.h>
#include <validation.h>

#include <cassert>

namespace {

void AssertTransactionFieldsEqual(const CTransaction& tx, const CMutableTransaction& mutable_tx)
{
    assert(tx.vin == mutable_tx.vin);
    assert(tx.vout == mutable_tx.vout);
    assert(tx.version == mutable_tx.version);
    assert(tx.nLockTime == mutable_tx.nLockTime);
    assert(tx.HasWitness() == mutable_tx.HasWitness());
}

} // namespace

void initialize_transaction()
{
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(transaction, .init = initialize_transaction)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    SpanReader ds{buffer};
    bool valid_tx = true;
    const CTransaction tx = [&] {
        try {
            return CTransaction(deserialize, TX_WITH_WITNESS, ds);
        } catch (const std::ios_base::failure&) {
            valid_tx = false;
            return CTransaction{CMutableTransaction{}};
        }
    }();
    bool valid_mutable_tx = true;
    CMutableTransaction mutable_tx;
    try {
        SpanReader{buffer} >> TX_WITH_WITNESS(mutable_tx);
    } catch (const std::ios_base::failure&) {
        valid_mutable_tx = false;
    }
    assert(valid_tx == valid_mutable_tx);
    if (!valid_tx) {
        return;
    }

    AssertTransactionFieldsEqual(tx, mutable_tx);

    DataStream serialized_with_witness;
    serialized_with_witness << TX_WITH_WITNESS(tx);
    DataStream serialized_without_witness;
    serialized_without_witness << TX_NO_WITNESS(tx);
    assert(serialized_with_witness.size() == ::GetSerializeSize(TX_WITH_WITNESS(tx)));
    assert(serialized_without_witness.size() == ::GetSerializeSize(TX_NO_WITNESS(tx)));

    DataStream round_trip_stream;
    round_trip_stream << TX_WITH_WITNESS(tx);
    CMutableTransaction round_tripped;
    round_trip_stream >> TX_WITH_WITNESS(round_tripped);
    assert(round_trip_stream.empty());
    AssertTransactionFieldsEqual(tx, round_tripped);

    const Txid expected_txid{Txid::FromUint256((HashWriter{} << TX_NO_WITNESS(tx)).GetHash())};
    const Wtxid expected_wtxid{Wtxid::FromUint256((HashWriter{} << TX_WITH_WITNESS(tx)).GetHash())};
    assert(tx.GetHash() == expected_txid);
    assert(tx.GetWitnessHash() == expected_wtxid);
    assert(mutable_tx.GetHash() == expected_txid);

    const unsigned int total_size{tx.ComputeTotalSize()};
    assert(total_size == serialized_with_witness.size());
    const int64_t expected_weight{
        static_cast<int64_t>(serialized_without_witness.size()) * (WITNESS_SCALE_FACTOR - 1) +
        static_cast<int64_t>(serialized_with_witness.size())};
    const int32_t weight{GetTransactionWeight(tx)};
    assert(weight == expected_weight);
    assert(GetVirtualTransactionSize(tx) == (expected_weight + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);

    {
        TxValidationState state_with_dupe_check;
        const bool res{CheckTransaction(tx, state_with_dupe_check)};
        Assert(res == state_with_dupe_check.IsValid());
    }

    const CFeeRate dust_relay_fee{DUST_RELAY_TX_FEE};
    std::string reason;
    const bool is_standard_with_permit_bare_multisig = IsStandardTx(tx, std::nullopt, /* permit_bare_multisig= */ true, dust_relay_fee, reason);
    const bool is_standard_without_permit_bare_multisig = IsStandardTx(tx, std::nullopt, /* permit_bare_multisig= */ false, dust_relay_fee, reason);
    if (is_standard_without_permit_bare_multisig) {
        assert(is_standard_with_permit_bare_multisig);
    }

    try {
        (void)tx.GetValueOut();
    } catch (const std::runtime_error&) {
    }
    (void)tx.HasWitness();
    (void)tx.IsCoinBase();
    (void)tx.IsNull();
    (void)tx.ToString();

    (void)EncodeHexTx(tx);
    (void)GetLegacySigOpCount(tx);
    (void)IsFinalTx(tx, /* nBlockHeight= */ 1024, /* nBlockTime= */ 1024);
    (void)RecursiveDynamicUsage(tx);
    (void)SignalsOptInRBF(tx);

    const CCoinsViewCache coins_view_cache{&CoinsViewEmpty::Get()};
    (void)ValidateInputsStandardness(tx, coins_view_cache);
    (void)IsWitnessStandard(tx, coins_view_cache);

    if (total_size < 250'000) { // Avoid high memory usage (with msan) due to json encoding
        {
            UniValue u{UniValue::VOBJ};
            TxToUniv(tx, /*block_hash=*/uint256::ZERO, /*entry=*/u);
        }
        {
            UniValue u{UniValue::VOBJ};
            TxToUniv(tx, /*block_hash=*/uint256::ONE, /*entry=*/u);
        }
    }
}
