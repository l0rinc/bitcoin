// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockencodings.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {
bool StrictlyIncreasing(std::span<const uint16_t> indexes)
{
    for (size_t i{1}; i < indexes.size(); ++i) {
        if (indexes[i] <= indexes[i - 1]) return false;
    }
    return true;
}

void AssertBlockTxnRequestRoundTrip(const BlockTransactionsRequest& request)
{
    DataStream serialized{};
    serialized << request;

    BlockTransactionsRequest decoded;
    serialized >> decoded;
    assert(serialized.empty());
    assert(decoded.blockhash == request.blockhash);
    assert(decoded.indexes == request.indexes);
}
} // namespace

FUZZ_TARGET(difference_formatter)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto block_hash = uint256::ONE;

    DataStream ss{};
    ss << block_hash << std::span{buffer};

    // Test deserialization
    try {
        BlockTransactionsRequest test_container;
        ss >> test_container;
        assert(test_container.blockhash == block_hash);

        // Invariant: strictly monotonic increasing (no duplicates allowed)
        for (size_t i = 1; i < test_container.indexes.size(); ++i) {
            assert(test_container.indexes[i] > test_container.indexes[i-1]);
        }
        AssertBlockTxnRequestRoundTrip(test_container);

    } catch (const std::ios_base::failure&) {
        // Expected for malformed input
    }

    BlockTransactionsRequest constructed;
    constructed.blockhash = block_hash;
    constructed.indexes = ConsumeRandomLengthIntegralVector<uint16_t>(fuzzed_data_provider, /*max_vector_size=*/64);

    try {
        AssertBlockTxnRequestRoundTrip(constructed);
        assert(StrictlyIncreasing(constructed.indexes));
    } catch (const std::ios_base::failure&) {
        assert(!StrictlyIncreasing(constructed.indexes));
    }
}
