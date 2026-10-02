// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/block_witness.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ios>
#include <optional>
#include <span>
#include <utility>
#include <vector>

FUZZ_TARGET(block_witness)
{
    const auto bytes{std::as_bytes(buffer)};
    std::vector<std::byte> data{bytes.begin(), bytes.end()};
    CBlock block;
    bool decoded{false};
    try {
        SpanReader{data} >> TX_WITH_WITNESS(block);
        decoded = true;
    } catch (const std::ios_base::failure&) {
    }
    bool stripped{false};
    try {
        data.resize(StripWitness(data));
        stripped = true;
    } catch (const std::ios_base::failure&) {
    }
    assert(decoded == stripped);
    if (decoded) {
        DataStream expected;
        expected << TX_NO_WITNESS(block);
        CBlock roundtrip;
        SpanReader{expected} >> TX_NO_WITNESS(roundtrip);
        DataStream actual;
        actual << TX_NO_WITNESS(roundtrip);
        assert(std::ranges::equal(actual, expected));
        assert(std::ranges::equal(data, expected));
    }
}

FUZZ_TARGET(block_witness_roundtrip)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    CBlock block;
    const auto count{provider.ConsumeIntegralInRange<uint8_t>(0, 16)};
    for (size_t i{0}; i < count; ++i) {
        auto tx{ConsumeTransaction(provider, std::nullopt)};
        if (tx.vin.empty()) tx.vout.clear(); // Empty vin with nonempty vout is ambiguous in witness serialization
        block.vtx.push_back(MakeTransactionRef(std::move(tx)));
    }
    DataStream full, expected;
    full << TX_WITH_WITNESS(block);
    expected << TX_NO_WITNESS(block);
    std::vector<std::byte> data{full.begin(), full.end()};
    const auto trailing{ConsumeRandomLengthByteVector<std::byte>(provider)};
    data.insert(data.end(), trailing.begin(), trailing.end());
    CBlock decoded;
    SpanReader{data} >> TX_WITH_WITNESS(decoded);
    DataStream actual;
    actual << TX_NO_WITNESS(decoded);
    assert(std::ranges::equal(actual, expected));
    SpanReader{actual} >> TX_NO_WITNESS(decoded);
    DataStream roundtrip;
    roundtrip << TX_NO_WITNESS(decoded);
    assert(std::ranges::equal(roundtrip, expected));
    data.resize(StripWitness(data));
    assert(std::ranges::equal(data, expected));
    assert(StripWitness(data) == data.size());
    assert(std::ranges::equal(data, expected));
}
