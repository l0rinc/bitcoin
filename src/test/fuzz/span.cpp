// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(span)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    std::string str = fuzzed_data_provider.ConsumeBytesAsString(32);
    const std::span<const char> span{str};
    (void)span.data();
    (void)span.begin();
    (void)span.end();
    if (span.size() > 0) {
        const std::ptrdiff_t idx = fuzzed_data_provider.ConsumeIntegralInRange<std::ptrdiff_t>(0U, span.size() - 1U);
        (void)span.first(idx);
        (void)span.last(idx);
        (void)span.subspan(idx);
        (void)span.subspan(idx, span.size() - idx);
        (void)span[idx];
    }

    std::vector<uint8_t> bytes = fuzzed_data_provider.ConsumeBytes<uint8_t>(32);
    std::vector<uint8_t> original{bytes};
    std::span<uint8_t> mutable_span{bytes};
    while (!mutable_span.empty() && fuzzed_data_provider.ConsumeBool()) {
        const size_t old_size{mutable_span.size()};
        uint8_t& popped{SpanPopBack(mutable_span)};
        assert(&popped == &bytes[old_size - 1]);
        assert(popped == original[old_size - 1]);
        assert(mutable_span.size() == old_size - 1);
        for (size_t i{0}; i < mutable_span.size(); ++i) {
            assert(mutable_span[i] == original[i]);
        }
        const uint8_t mutated{static_cast<uint8_t>(popped ^ uint8_t{0x5a})};
        popped = mutated;
        assert(bytes[old_size - 1] == mutated);
    }
}
