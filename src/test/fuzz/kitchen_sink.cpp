// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/messages.h>
#include <merkleblock.h>
#include <node/types.h>
#include <policy/fees/block_policy_estimator.h>
#include <rpc/protocol.h>
#include <rpc/util.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/translation.h>
#include <univalue.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using common::TransactionErrorString;
using node::TransactionError;

namespace {
constexpr TransactionError ALL_TRANSACTION_ERROR[] = {
    TransactionError::MISSING_INPUTS,
    TransactionError::ALREADY_IN_UTXO_SET,
    TransactionError::MEMPOOL_REJECTED,
    TransactionError::MEMPOOL_ERROR,
    TransactionError::MAX_FEE_EXCEEDED,
    TransactionError::MAX_BURN_EXCEEDED,
    TransactionError::INVALID_PACKAGE,
};

RPCErrorCode ExpectedRPCErrorCode(const TransactionError error)
{
    switch (error) {
    case TransactionError::MEMPOOL_REJECTED:
        return RPC_TRANSACTION_REJECTED;
    case TransactionError::ALREADY_IN_UTXO_SET:
        return RPC_VERIFY_ALREADY_IN_UTXO_SET;
    case TransactionError::MISSING_INPUTS:
    case TransactionError::MEMPOOL_ERROR:
    case TransactionError::MAX_FEE_EXCEEDED:
    case TransactionError::MAX_BURN_EXCEEDED:
    case TransactionError::INVALID_PACKAGE:
        return RPC_TRANSACTION_ERROR;
    case TransactionError::OK:
        break;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
    return RPC_TRANSACTION_ERROR;
}

void AssertJSONRPCError(const UniValue& error, const RPCErrorCode expected_code, const std::string_view expected_message)
{
    assert(error.isObject());
    assert(error.size() == 2);
    assert(error.exists("code"));
    assert(error.exists("message"));
    assert(error["code"].isNum());
    assert(error["message"].isStr());
    assert(error["code"].getInt<int>() == expected_code);
    assert(error["message"].get_str() == expected_message);
}

void AssertTransactionErrorContracts(const TransactionError error, const std::string& custom_message)
{
    const bilingual_str message{TransactionErrorString(error)};
    assert(!message.original.empty());

    const RPCErrorCode expected_code{ExpectedRPCErrorCode(error)};
    assert(RPCErrorFromTransactionError(error) == expected_code);
    AssertJSONRPCError(JSONRPCTransactionError(error), expected_code, message.original);
    AssertJSONRPCError(
        JSONRPCTransactionError(error, custom_message),
        expected_code,
        custom_message.empty() ? std::string_view{message.original} : std::string_view{custom_message});
}
}; // namespace

// The fuzzing kitchen sink: Fuzzing harness for functions that need to be
// fuzzed but a.) don't belong in any existing fuzzing harness file, and
// b.) are not important enough to warrant their own fuzzing harness file.
FUZZ_TARGET(kitchen_sink)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const TransactionError transaction_error = fuzzed_data_provider.PickValueInArray(ALL_TRANSACTION_ERROR);
    AssertTransactionErrorContracts(
        transaction_error,
        fuzzed_data_provider.ConsumeRandomLengthString(64));

    (void)StringForFeeEstimateHorizon(fuzzed_data_provider.PickValueInArray(ALL_FEE_ESTIMATE_HORIZONS));

    const OutputType output_type = fuzzed_data_provider.PickValueInArray(OUTPUT_TYPES);
    const std::string& output_type_string = FormatOutputType(output_type);
    const std::optional<OutputType> parsed = ParseOutputType(output_type_string);
    assert(parsed);
    assert(output_type == parsed.value());
    (void)ParseOutputType(fuzzed_data_provider.ConsumeRandomLengthString(64));

    const std::vector<uint8_t> bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::vector<bool> bits = BytesToBits(bytes);
    const std::vector<uint8_t> bytes_decoded = BitsToBytes(bits);
    assert(bytes == bytes_decoded);

    const size_t bit_count{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 512)};
    std::vector<bool> arbitrary_bits;
    arbitrary_bits.reserve(bit_count);
    for (size_t i{0}; i < bit_count; ++i) {
        arbitrary_bits.push_back(fuzzed_data_provider.ConsumeBool());
    }
    const std::vector<uint8_t> encoded_bits{BitsToBytes(arbitrary_bits)};
    const std::vector<bool> decoded_bits{BytesToBits(encoded_bits)};
    assert(decoded_bits.size() == encoded_bits.size() * 8);
    assert(decoded_bits.size() >= arbitrary_bits.size());
    assert(std::equal(arbitrary_bits.begin(), arbitrary_bits.end(), decoded_bits.begin()));
    assert(std::all_of(decoded_bits.begin() + arbitrary_bits.size(), decoded_bits.end(),
        [](bool bit) { return !bit; }));
    assert(BitsToBytes(decoded_bits) == encoded_bits);
}
