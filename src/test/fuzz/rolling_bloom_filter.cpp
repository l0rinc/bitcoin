// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bloom.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace {

void ExerciseGenerationRollover()
{
    constexpr unsigned int N_ELEMENTS{7};
    CRollingBloomFilter filter{N_ELEMENTS, 0.01};
    std::deque<std::vector<unsigned char>> recent;
    for (unsigned int i{0}; i < 4 * N_ELEMENTS; ++i) {
        const std::vector<unsigned char> key{0xa5, static_cast<unsigned char>(i), 0x5a};
        filter.insert(key);
        recent.push_back(key);
        if (recent.size() > N_ELEMENTS) recent.pop_front();
        for (const auto& retained : recent) {
            assert(filter.contains(retained));
        }
    }

    filter.reset();
    const std::vector<unsigned char> post_reset_key{0x42, 0x24};
    filter.insert(post_reset_key);
    assert(filter.contains(post_reset_key));
}

} // namespace

FUZZ_TARGET(rolling_bloom_filter)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    ExerciseGenerationRollover();
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const unsigned int n_elements{fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 1000)};
    CRollingBloomFilter rolling_bloom_filter{
        n_elements,
        0.999 / fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, std::numeric_limits<unsigned int>::max())};
    using InsertedKey = std::variant<std::vector<unsigned char>, uint256>;
    std::deque<InsertedKey> recent;
    const auto assert_recent = [&] {
        for (const auto& retained : recent) {
            std::visit([&](const auto& key) { assert(rolling_bloom_filter.contains(key)); }, retained);
        }
    };
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 3000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                rolling_bloom_filter.insert(b);
                recent.push_back(b);
                if (recent.size() > n_elements) recent.pop_front();
                assert_recent();
            },
            [&] {
                const uint256 u256{ConsumeUInt256(fuzzed_data_provider)};
                (void)rolling_bloom_filter.contains(u256);
                rolling_bloom_filter.insert(u256);
                recent.push_back(u256);
                if (recent.size() > n_elements) recent.pop_front();
                assert_recent();
            },
            [&] {
                rolling_bloom_filter.reset();
                recent.clear();
            });
    }
}
