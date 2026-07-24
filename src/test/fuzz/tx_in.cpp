// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>

namespace {

CScriptWitness ConsumeBoundedWitness(FuzzedDataProvider& fuzzed_data_provider)
{
    CScriptWitness witness;
    const size_t num_elements{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4)};
    for (size_t i{0}; i < num_elements; ++i) {
        witness.stack.push_back(ConsumeRandomLengthByteVector(fuzzed_data_provider, /*max_length=*/32));
    }
    return witness;
}

} // namespace

FUZZ_TARGET(tx_in)
{
    CTxIn tx_in;
    try {
        SpanReader{buffer} >> tx_in;
    } catch (const std::ios_base::failure&) {
        return;
    }

    // CTxIn serialization intentionally excludes the witness; a transaction
    // envelope is required to exercise the witness round trip.
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    tx_in.scriptWitness = ConsumeBoundedWitness(fuzzed_data_provider);

    DataStream serialized_input;
    serialized_input << tx_in;
    assert(serialized_input.size() == ::GetSerializeSize(tx_in));
    CTxIn round_tripped_input;
    serialized_input >> round_tripped_input;
    assert(serialized_input.empty());
    assert(round_tripped_input == tx_in);
    assert(round_tripped_input.scriptWitness.IsNull());

    CMutableTransaction transaction;
    transaction.vin.push_back(tx_in);
    DataStream serialized_transaction;
    serialized_transaction << TX_WITH_WITNESS(transaction);
    CMutableTransaction round_tripped_transaction;
    serialized_transaction >> TX_WITH_WITNESS(round_tripped_transaction);
    assert(serialized_transaction.empty());
    assert(round_tripped_transaction.vin.size() == 1);
    assert(round_tripped_transaction.vin.front() == tx_in);
    assert(round_tripped_transaction.vin.front().scriptWitness == tx_in.scriptWitness);

    const int64_t stripped_size{static_cast<int64_t>(GetSerializeSize(TX_NO_WITNESS(tx_in)))};
    const int64_t total_input_size{static_cast<int64_t>(GetSerializeSize(TX_WITH_WITNESS(tx_in)))};
    const int64_t witness_size{static_cast<int64_t>(GetSerializeSize(tx_in.scriptWitness.stack))};
    const int64_t expected_weight{stripped_size * (WITNESS_SCALE_FACTOR - 1) + total_input_size + witness_size};
    assert(GetTransactionInputWeight(tx_in) == expected_weight);
    assert(GetVirtualTransactionInputSize(tx_in) == (expected_weight + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);
    (void)RecursiveDynamicUsage(tx_in);

    (void)tx_in.ToString();
}
