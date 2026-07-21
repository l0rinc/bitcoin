// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cuckoocache.h>
#include <script/sigcache.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/byte_units.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <vector>

namespace {
FuzzedDataProvider* fuzzed_data_provider_ptr = nullptr;

struct RandomHasher {
private:
    static uint32_t ConsumeSeed()
    {
        assert(fuzzed_data_provider_ptr != nullptr);
        return fuzzed_data_provider_ptr->ConsumeIntegral<uint32_t>();
    }

    const uint32_t m_seed{ConsumeSeed()};

public:
    template <uint8_t hash_select>
    uint32_t operator()(const uint32_t& element) const
    {
        uint32_t value{m_seed + 0x9e3779b9U * (hash_select + 1) + element * 0x85ebca6bU};
        value ^= value >> 16;
        value *= 0x7feb352dU;
        value ^= value >> 15;
        value *= 0x846ca68bU;
        return value ^ (value >> 16);
    }
};

using Cache = CuckooCache::cache<uint32_t, RandomHasher>;

void CheckNoFalsePositive(const Cache& cache, const std::set<uint32_t>& ever_inserted, uint32_t element)
{
    assert(!cache.contains(element, /*erase=*/false) || ever_inserted.contains(element));
}
} // namespace

FUZZ_TARGET(cuckoocache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    fuzzed_data_provider_ptr = &fuzzed_data_provider;
    Cache cuckoo_cache{};
    std::set<uint32_t> ever_inserted;
    if (fuzzed_data_provider.ConsumeBool()) {
        const size_t megabytes = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 16);
        const auto [num_elements, approx_size_bytes] = cuckoo_cache.setup_bytes(megabytes * 1_MiB);
        assert(num_elements == std::max<size_t>(2, megabytes * 1_MiB / sizeof(uint32_t)));
        assert(approx_size_bytes == num_elements * sizeof(uint32_t));
    } else {
        const uint32_t requested_size = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096);
        assert(cuckoo_cache.setup(requested_size) == std::max<uint32_t>(2, requested_size));
    }
    if (!ever_inserted.contains(std::numeric_limits<uint32_t>::max())) {
        CheckNoFalsePositive(cuckoo_cache, ever_inserted, std::numeric_limits<uint32_t>::max());
    }
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        if (fuzzed_data_provider.ConsumeBool()) {
            const uint32_t element{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, std::numeric_limits<uint32_t>::max())};
            cuckoo_cache.insert(element);
            ever_inserted.insert(element);
        } else {
            const uint32_t element{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, std::numeric_limits<uint32_t>::max())};
            const bool erase{fuzzed_data_provider.ConsumeBool()};
            const bool found{cuckoo_cache.contains(element, erase)};
            assert(!found || ever_inserted.contains(element));
        }
    }
    fuzzed_data_provider_ptr = nullptr;
}
