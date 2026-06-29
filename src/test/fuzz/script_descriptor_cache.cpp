// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

void AssertParentCacheEntry(const DescriptorCache& cache, uint32_t key_exp_pos, const CExtPubKey& expected)
{
    CExtPubKey actual;
    assert(cache.GetCachedParentExtPubKey(key_exp_pos, actual));
    assert(actual == expected);
}

void AssertDerivedCacheEntry(const DescriptorCache& cache, uint32_t key_exp_pos, uint32_t der_index, const CExtPubKey& expected)
{
    CExtPubKey actual;
    assert(cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, actual));
    assert(actual == expected);
}

void AssertLastHardenedCacheEntry(const DescriptorCache& cache, uint32_t key_exp_pos, const CExtPubKey& expected)
{
    CExtPubKey actual;
    assert(cache.GetCachedLastHardenedExtPubKey(key_exp_pos, actual));
    assert(actual == expected);
}

} // namespace

FUZZ_TARGET(script_descriptor_cache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    DescriptorCache descriptor_cache;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> code = fuzzed_data_provider.ConsumeBytes<uint8_t>(BIP32_EXTKEY_SIZE);
        if (code.size() == BIP32_EXTKEY_SIZE) {
            CExtPubKey xpub;
            xpub.Decode(code.data());
            const uint32_t key_exp_pos = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            CExtPubKey xpub_fetched;
            const uint32_t der_index = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    (void)descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched);
                    descriptor_cache.CacheParentExtPubKey(key_exp_pos, xpub);
                    AssertParentCacheEntry(descriptor_cache, key_exp_pos, xpub);
                },
                [&] {
                    (void)descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched);
                    descriptor_cache.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub);
                    AssertDerivedCacheEntry(descriptor_cache, key_exp_pos, der_index, xpub);
                },
                [&] {
                    (void)descriptor_cache.GetCachedLastHardenedExtPubKey(key_exp_pos, xpub_fetched);
                    descriptor_cache.CacheLastHardenedExtPubKey(key_exp_pos, xpub);
                    AssertLastHardenedCacheEntry(descriptor_cache, key_exp_pos, xpub);
                },
                [&] {
                    DescriptorCache other_cache;
                    DescriptorCache expected_diff;

                    if (descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched)) {
                        other_cache.CacheParentExtPubKey(key_exp_pos, xpub_fetched);
                    } else {
                        other_cache.CacheParentExtPubKey(key_exp_pos, xpub);
                        expected_diff.CacheParentExtPubKey(key_exp_pos, xpub);
                    }

                    if (descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched)) {
                        other_cache.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched);
                    } else {
                        other_cache.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub);
                        expected_diff.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub);
                    }

                    if (descriptor_cache.GetCachedLastHardenedExtPubKey(key_exp_pos, xpub_fetched)) {
                        other_cache.CacheLastHardenedExtPubKey(key_exp_pos, xpub_fetched);
                    } else {
                        other_cache.CacheLastHardenedExtPubKey(key_exp_pos, xpub);
                        expected_diff.CacheLastHardenedExtPubKey(key_exp_pos, xpub);
                    }

                    const DescriptorCache diff{descriptor_cache.MergeAndDiff(other_cache)};
                    assert(diff.GetCachedParentExtPubKeys() == expected_diff.GetCachedParentExtPubKeys());
                    assert(diff.GetCachedDerivedExtPubKeys() == expected_diff.GetCachedDerivedExtPubKeys());
                    assert(diff.GetCachedLastHardenedExtPubKeys() == expected_diff.GetCachedLastHardenedExtPubKeys());

                    assert(other_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched));
                    AssertParentCacheEntry(descriptor_cache, key_exp_pos, xpub_fetched);
                    assert(other_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched));
                    AssertDerivedCacheEntry(descriptor_cache, key_exp_pos, der_index, xpub_fetched);
                    assert(other_cache.GetCachedLastHardenedExtPubKey(key_exp_pos, xpub_fetched));
                    AssertLastHardenedCacheEntry(descriptor_cache, key_exp_pos, xpub_fetched);
                });
        }
        (void)descriptor_cache.GetCachedParentExtPubKeys();
        (void)descriptor_cache.GetCachedDerivedExtPubKeys();
        (void)descriptor_cache.GetCachedLastHardenedExtPubKeys();
    }
}
