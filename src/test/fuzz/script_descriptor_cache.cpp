// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using ParentModel = std::map<uint32_t, CExtPubKey>;
using DerivedModel = std::map<uint32_t, std::map<uint32_t, CExtPubKey>>;

struct CacheModel {
    ParentModel parent;
    DerivedModel derived;
    ParentModel last_hardened;
};

template <typename Map>
uint32_t FindUnusedKey(const Map& entries, uint32_t candidate)
{
    while (entries.contains(candidate)) {
        assert(candidate != 0);
        --candidate;
    }
    return candidate;
}

void AssertCacheContracts(const DescriptorCache& cache, const CacheModel& expected)
{
    const auto parents{cache.GetCachedParentExtPubKeys()};
    assert(parents.size() == expected.parent.size());
    for (const auto& [key_exp_pos, xpub] : expected.parent) {
        const auto it{parents.find(key_exp_pos)};
        assert(it != parents.end());
        assert(it->second == xpub);
    }

    const auto derived{cache.GetCachedDerivedExtPubKeys()};
    assert(derived.size() == expected.derived.size());
    for (const auto& [key_exp_pos, expected_xpubs] : expected.derived) {
        const auto key_exp_it{derived.find(key_exp_pos)};
        assert(key_exp_it != derived.end());
        assert(key_exp_it->second.size() == expected_xpubs.size());
        for (const auto& [der_index, xpub] : expected_xpubs) {
            const auto der_it{key_exp_it->second.find(der_index)};
            assert(der_it != key_exp_it->second.end());
            assert(der_it->second == xpub);
        }
    }

    const auto last_hardened{cache.GetCachedLastHardenedExtPubKeys()};
    assert(last_hardened.size() == expected.last_hardened.size());
    for (const auto& [key_exp_pos, xpub] : expected.last_hardened) {
        const auto it{last_hardened.find(key_exp_pos)};
        assert(it != last_hardened.end());
        assert(it->second == xpub);
    }
}
} // namespace

FUZZ_TARGET(script_descriptor_cache)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    DescriptorCache descriptor_cache;
    CacheModel expected;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> code = fuzzed_data_provider.ConsumeBytes<uint8_t>(BIP32_EXTKEY_SIZE);
        if (code.size() == BIP32_EXTKEY_SIZE) {
            CExtPubKey xpub;
            xpub.Decode(code.data());
            const uint32_t key_exp_pos = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
            CExtPubKey xpub_fetched;
            if (fuzzed_data_provider.ConsumeBool()) {
                const bool was_cached{descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched)};
                assert(was_cached == expected.parent.contains(key_exp_pos));
                descriptor_cache.CacheParentExtPubKey(key_exp_pos, xpub);
                expected.parent[key_exp_pos] = xpub;
                assert(descriptor_cache.GetCachedParentExtPubKey(key_exp_pos, xpub_fetched));
            } else if (fuzzed_data_provider.ConsumeBool()) {
                const uint32_t der_index = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
                const bool was_cached{descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched)};
                const auto key_exp_it{expected.derived.find(key_exp_pos)};
                const bool expected_cached{key_exp_it != expected.derived.end() && key_exp_it->second.contains(der_index)};
                assert(was_cached == expected_cached);
                descriptor_cache.CacheDerivedExtPubKey(key_exp_pos, der_index, xpub);
                expected.derived[key_exp_pos][der_index] = xpub;
                assert(descriptor_cache.GetCachedDerivedExtPubKey(key_exp_pos, der_index, xpub_fetched));
            } else {
                const bool was_cached{descriptor_cache.GetCachedLastHardenedExtPubKey(key_exp_pos, xpub_fetched)};
                assert(was_cached == expected.last_hardened.contains(key_exp_pos));
                descriptor_cache.CacheLastHardenedExtPubKey(key_exp_pos, xpub);
                expected.last_hardened[key_exp_pos] = xpub;
                assert(descriptor_cache.GetCachedLastHardenedExtPubKey(key_exp_pos, xpub_fetched));
            }
            assert(xpub == xpub_fetched);
        }
        AssertCacheContracts(descriptor_cache, expected);
        (void)descriptor_cache.GetCachedParentExtPubKeys();
        (void)descriptor_cache.GetCachedDerivedExtPubKeys();
    }

    DescriptorCache incoming;
    CacheModel incoming_expected;
    const CExtPubKey merge_xpub{};
    const uint32_t parent_key{FindUnusedKey(expected.parent, std::numeric_limits<uint32_t>::max())};
    const uint32_t derived_key{FindUnusedKey(expected.derived, std::numeric_limits<uint32_t>::max() - 1)};
    const uint32_t last_hardened_key{FindUnusedKey(expected.last_hardened, std::numeric_limits<uint32_t>::max() - 2)};
    incoming.CacheParentExtPubKey(parent_key, merge_xpub);
    incoming_expected.parent[parent_key] = merge_xpub;
    incoming.CacheDerivedExtPubKey(derived_key, std::numeric_limits<uint32_t>::max(), merge_xpub);
    incoming_expected.derived[derived_key][std::numeric_limits<uint32_t>::max()] = merge_xpub;
    incoming.CacheLastHardenedExtPubKey(last_hardened_key, merge_xpub);
    incoming_expected.last_hardened[last_hardened_key] = merge_xpub;

    const DescriptorCache diff{descriptor_cache.MergeAndDiff(incoming)};
    expected.parent[parent_key] = merge_xpub;
    expected.derived[derived_key][std::numeric_limits<uint32_t>::max()] = merge_xpub;
    expected.last_hardened[last_hardened_key] = merge_xpub;
    AssertCacheContracts(descriptor_cache, expected);
    AssertCacheContracts(diff, incoming_expected);

    DescriptorCache conflicting;
    CExtPubKey conflicting_xpub{};
    conflicting_xpub.nDepth = 1;
    conflicting.CacheParentExtPubKey(parent_key, conflicting_xpub);
    try {
        (void)descriptor_cache.MergeAndDiff(conflicting);
        assert(false);
    } catch (const std::runtime_error&) {
    }
    AssertCacheContracts(descriptor_cache, expected);
}
