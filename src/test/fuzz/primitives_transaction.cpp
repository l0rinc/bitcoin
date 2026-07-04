// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace {
void AssertTransactionCachedHashContracts(const CTransaction& tx)
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

void AssertTransactionSerializationContracts(const CTransaction& tx)
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

FUZZ_TARGET(primitives_transaction)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CScript script = ConsumeScript(fuzzed_data_provider);
    const std::optional<COutPoint> out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
    if (out_point) {
        const CTxIn tx_in{*out_point, script, fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
        (void)tx_in;
    }
    const CTxOut tx_out_1{ConsumeMoney(fuzzed_data_provider), script};
    const CTxOut tx_out_2{ConsumeMoney(fuzzed_data_provider), ConsumeScript(fuzzed_data_provider)};
    assert((tx_out_1 == tx_out_2) != (tx_out_1 != tx_out_2));
    {
        CMutableTransaction constructed_tx;
        constructed_tx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), 0}, CScript{}, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
        constructed_tx.vout.emplace_back(tx_out_1.nValue, CScript{});

        const CTransaction no_witness_tx{constructed_tx};
        AssertTransactionCachedHashContracts(no_witness_tx);
        AssertTransactionSerializationContracts(no_witness_tx);
        assert(!no_witness_tx.HasWitness());

        constructed_tx.vin[0].scriptWitness.stack.push_back(ConsumeRandomLengthByteVector(fuzzed_data_provider, 64));
        const CTransaction witness_tx{constructed_tx};
        AssertTransactionCachedHashContracts(witness_tx);
        AssertTransactionSerializationContracts(witness_tx);
        assert(witness_tx.HasWitness());
        assert(witness_tx.GetHash() == no_witness_tx.GetHash());
    }
    const std::optional<CMutableTransaction> mutable_tx_1 = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    const std::optional<CMutableTransaction> mutable_tx_2 = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (mutable_tx_1) {
        const CTransaction tx_1{*mutable_tx_1};
        AssertTransactionCachedHashContracts(tx_1);
        AssertTransactionSerializationContracts(tx_1);
        if (!mutable_tx_2) return;
        const CTransaction tx_2{*mutable_tx_2};
        AssertTransactionCachedHashContracts(tx_2);
        AssertTransactionSerializationContracts(tx_2);
        assert((tx_1 == tx_2) != (tx_1 != tx_2));
    }
}
