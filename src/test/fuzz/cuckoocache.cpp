// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cuckoocache.h>
#include <script/sigcache.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/byte_units.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {
FuzzedDataProvider* fuzzed_data_provider_ptr = nullptr;

struct RandomHasher {
    template <uint8_t>
    uint32_t operator()(const bool& /* unused */) const
    {
        assert(fuzzed_data_provider_ptr != nullptr);
        return fuzzed_data_provider_ptr->ConsumeIntegral<uint32_t>();
    }
};
} // namespace

FUZZ_TARGET(cuckoocache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    fuzzed_data_provider_ptr = &fuzzed_data_provider;
    CuckooCache::cache<int, RandomHasher> cuckoo_cache{};
    CuckooCache::cache<uint256, SignatureCacheHasher> deterministic_cache{};
    if (fuzzed_data_provider.ConsumeBool()) {
        const size_t megabytes = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 16);
        cuckoo_cache.setup_bytes(megabytes * 1_MiB);
        deterministic_cache.setup_bytes(megabytes * 1_MiB);
    } else {
        const uint32_t size{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, 4096)};
        cuckoo_cache.setup(size);
        deterministic_cache.setup(size);
    }
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                cuckoo_cache.insert(fuzzed_data_provider.ConsumeBool());
            },
            [&] {
                const auto e{fuzzed_data_provider.ConsumeBool()};
                const auto erase{fuzzed_data_provider.ConsumeBool()};
                cuckoo_cache.contains(e, erase);
            },
            [&] {
                deterministic_cache.insert(ConsumeUInt256(fuzzed_data_provider));
            },
            [&] {
                const uint256 e{ConsumeUInt256(fuzzed_data_provider)};
                const bool contained{deterministic_cache.contains(e, fuzzed_data_provider.ConsumeBool())};
                if (contained) {
                    assert(deterministic_cache.contains(e, /*erase=*/false));
                }
             });
    }
    fuzzed_data_provider_ptr = nullptr;
}
