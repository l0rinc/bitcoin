// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_memusage.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <optional>

FUZZ_TARGET(tx_in)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::optional<CTxIn> tx_in{ConsumeDeserializable<CTxIn>(fuzzed_data_provider)};
    if (!tx_in) {
        return;
    }
    tx_in->scriptWitness = ConsumeScriptWitness(fuzzed_data_provider);

    const auto no_witness_size{GetSerializeSize(TX_NO_WITNESS(*tx_in))};
    const auto with_witness_size{GetSerializeSize(TX_WITH_WITNESS(*tx_in))};
    assert(no_witness_size == with_witness_size);

    const int64_t input_weight{GetTransactionInputWeight(*tx_in)};
    const int64_t expected_weight{
        static_cast<int64_t>(no_witness_size) * (WITNESS_SCALE_FACTOR - 1) +
        static_cast<int64_t>(with_witness_size) +
        static_cast<int64_t>(GetSerializeSize(tx_in->scriptWitness.stack))};
    assert(input_weight == expected_weight);
    assert(GetVirtualTransactionInputSize(*tx_in) == GetVirtualTransactionSize(input_weight, /*nSigOpCost=*/0, /*bytes_per_sigop=*/0));
    (void)RecursiveDynamicUsage(*tx_in);

    (void)tx_in->ToString();
}
