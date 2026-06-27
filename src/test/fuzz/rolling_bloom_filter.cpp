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
#include <vector>

FUZZ_TARGET(rolling_bloom_filter)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    const unsigned int n_elements{fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 1000)};
    CRollingBloomFilter rolling_bloom_filter{
        n_elements,
        0.999 / fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, std::numeric_limits<unsigned int>::max())};
    std::deque<std::vector<unsigned char>> recent_keys;
    auto remember_recent_key = [&](std::vector<unsigned char> key) {
        recent_keys.push_back(std::move(key));
        if (recent_keys.size() > n_elements) recent_keys.pop_front();
    };
    auto assert_recent_keys_present = [&] {
        for (const auto& key : recent_keys) {
            assert(rolling_bloom_filter.contains(key));
        }
    };
    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 3000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                const bool present_before = rolling_bloom_filter.contains(b);
                if (recent_keys.empty()) {
                    assert(!present_before);
                }
                rolling_bloom_filter.insert(b);
                const bool present = rolling_bloom_filter.contains(b);
                assert(present);
                remember_recent_key(b);
            },
            [&] {
                const uint256 u256{ConsumeUInt256(fuzzed_data_provider)};
                const std::vector<unsigned char> key{u256.begin(), u256.end()};
                const bool present_before = rolling_bloom_filter.contains(key);
                if (recent_keys.empty()) {
                    assert(!present_before);
                }
                rolling_bloom_filter.insert(key);
                const bool present = rolling_bloom_filter.contains(key);
                assert(present);
                remember_recent_key(key);
            },
            [&] {
                rolling_bloom_filter.reset();
                recent_keys.clear();
            });
        assert_recent_keys_present();
    }
}
