// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>
#include <test/util/random.h>
#include <univalue.h>
#include <util/chaintype.h>
#include <util/rbf.h>
#include <validation.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <string_view>

namespace {

void AssertCachedTransactionState(const CTransaction& tx)
{
    const Txid expected_txid{Txid::FromUint256((HashWriter{} << TX_NO_WITNESS(tx)).GetHash())};
    assert(tx.GetHash() == expected_txid);

    const bool expected_has_witness{std::ranges::any_of(tx.vin, [](const CTxIn& input) {
        return !input.scriptWitness.IsNull();
    })};
    assert(tx.HasWitness() == expected_has_witness);

    const Wtxid expected_wtxid{tx.HasWitness() ?
        Wtxid::FromUint256((HashWriter{} << TX_WITH_WITNESS(tx)).GetHash()) :
        Wtxid::FromUint256(tx.GetHash().ToUint256())};
    assert(tx.GetWitnessHash() == expected_wtxid);

    CMutableTransaction mutable_tx{tx};
    assert(mutable_tx.GetHash() == tx.GetHash());
    assert(mutable_tx.HasWitness() == tx.HasWitness());

    const CTransaction rebuilt_tx{mutable_tx};
    assert(rebuilt_tx.GetHash() == tx.GetHash());
    assert(rebuilt_tx.GetWitnessHash() == tx.GetWitnessHash());
    assert(rebuilt_tx.HasWitness() == tx.HasWitness());
    assert(rebuilt_tx.ComputeTotalSize() == tx.ComputeTotalSize());
}

void AssertTransactionSerializationRoundTrip(const CTransaction& tx)
{
    DataStream with_witness_stream;
    with_witness_stream << TX_WITH_WITNESS(tx);
    assert(with_witness_stream.size() == GetSerializeSize(TX_WITH_WITNESS(tx)));
    SpanReader with_witness_reader{std::span<const std::byte>{with_witness_stream.data(), with_witness_stream.size()}};
    const CTransaction with_witness_roundtrip{deserialize, TX_WITH_WITNESS, with_witness_reader};
    assert(with_witness_reader.empty());
    assert(with_witness_roundtrip.GetHash() == tx.GetHash());
    assert(with_witness_roundtrip.GetWitnessHash() == tx.GetWitnessHash());
    assert(with_witness_roundtrip.HasWitness() == tx.HasWitness());

    DataStream no_witness_stream;
    no_witness_stream << TX_NO_WITNESS(tx);
    assert(no_witness_stream.size() == GetSerializeSize(TX_NO_WITNESS(tx)));
    SpanReader no_witness_reader{std::span<const std::byte>{no_witness_stream.data(), no_witness_stream.size()}};
    const CTransaction no_witness_roundtrip{deserialize, TX_NO_WITNESS, no_witness_reader};
    assert(no_witness_reader.empty());
    assert(no_witness_roundtrip.GetHash() == tx.GetHash());
    assert(!no_witness_roundtrip.HasWitness());
    assert(no_witness_roundtrip.GetWitnessHash().ToUint256() == no_witness_roundtrip.GetHash().ToUint256());
    if (!tx.HasWitness()) {
        assert(no_witness_roundtrip.GetWitnessHash() == tx.GetWitnessHash());
    }
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

    AssertCachedTransactionState(tx);
    AssertTransactionSerializationRoundTrip(tx);

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

    (void)tx.GetHash();
    (void)tx.ComputeTotalSize();
    CAmount value_out{0};
    bool value_out_in_range{true};
    for (const auto& tx_out : tx.vout) {
        if (!MoneyRange(tx_out.nValue) || !MoneyRange(value_out + tx_out.nValue)) {
            value_out_in_range = false;
            break;
        }
        value_out += tx_out.nValue;
    }
    try {
        const CAmount computed_value_out{tx.GetValueOut()};
        assert(value_out_in_range);
        assert(computed_value_out == value_out);
        assert(MoneyRange(computed_value_out));
    } catch (const std::runtime_error& e) {
        assert(!value_out_in_range);
        assert(std::string_view{e.what()} == "GetValueOut: value out of range");
    }
    (void)tx.GetWitnessHash();
    (void)tx.HasWitness();
    (void)tx.IsCoinBase();
    (void)tx.IsNull();
    (void)tx.ToString();

    (void)EncodeHexTx(tx);
    (void)GetLegacySigOpCount(tx);
    (void)GetTransactionWeight(tx);
    (void)GetVirtualTransactionSize(tx);
    (void)IsFinalTx(tx, /* nBlockHeight= */ 1024, /* nBlockTime= */ 1024);
    (void)RecursiveDynamicUsage(tx);
    (void)SignalsOptInRBF(tx);

    const CCoinsViewCache coins_view_cache{&CoinsViewEmpty::Get()};
    (void)ValidateInputsStandardness(tx, coins_view_cache);
    (void)IsWitnessStandard(tx, coins_view_cache);

    if (tx.ComputeTotalSize() < 250'000) { // Avoid high memory usage (with msan) due to json encoding
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
