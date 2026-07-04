// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <net.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

namespace {
bool EvictionEligible(const NodeEvictionCandidate& candidate)
{
    return !candidate.m_noban && candidate.m_conn_type == ConnectionType::INBOUND;
}

NodeEvictionCandidate ProtectedNoiseCandidate(NodeId id)
{
    return {
        /*id=*/id,
        /*m_connected=*/NodeClock::time_point::max(),
        /*m_min_ping_time=*/NodeClock::duration::max(),
        /*m_last_block_time=*/std::chrono::seconds::min(),
        /*m_last_tx_time=*/std::chrono::seconds::min(),
        /*fRelevantServices=*/false,
        /*m_relay_txs=*/true,
        /*fBloomFilter=*/false,
        /*nKeyedNetGroup=*/0,
        /*prefer_evict=*/true,
        /*m_is_local=*/false,
        /*m_network=*/NET_IPV4,
        /*m_noban=*/true,
        /*m_conn_type=*/ConnectionType::INBOUND,
    };
}

void AppendProtectedNoise(std::vector<NodeEvictionCandidate>& candidates)
{
    const NodeId first_noise_id{static_cast<NodeId>(candidates.size())};
    NodeEvictionCandidate noban{ProtectedNoiseCandidate(first_noise_id)};
    noban.m_noban = true;
    noban.m_conn_type = ConnectionType::INBOUND;
    candidates.push_back(noban);

    NodeEvictionCandidate outbound{ProtectedNoiseCandidate(first_noise_id + 1)};
    outbound.m_noban = false;
    outbound.m_conn_type = ConnectionType::OUTBOUND_FULL_RELAY;
    candidates.push_back(outbound);
}
} // namespace

FUZZ_TARGET(node_eviction)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<NodeEvictionCandidate> eviction_candidates;
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const NodeId id{static_cast<NodeId>(eviction_candidates.size())};
        (void)fuzzed_data_provider.ConsumeIntegral<NodeId>(); // Preserve the previous byte layout.
        eviction_candidates.push_back({
            /*id=*/id,
            /*m_connected=*/ConsumeTime(fuzzed_data_provider),
            /*m_min_ping_time=*/ConsumeDuration<decltype(NodeEvictionCandidate::m_min_ping_time)>(fuzzed_data_provider, /*min=*/std::chrono::years{-1}, /*max=*/decltype(CNode::m_min_ping_time.load())::max()),
            /*m_last_block_time=*/ConsumeTime(fuzzed_data_provider).time_since_epoch(),
            /*m_last_tx_time=*/ConsumeTime(fuzzed_data_provider).time_since_epoch(),
            /*fRelevantServices=*/fuzzed_data_provider.ConsumeBool(),
            /*m_relay_txs=*/fuzzed_data_provider.ConsumeBool(),
            /*fBloomFilter=*/fuzzed_data_provider.ConsumeBool(),
            /*nKeyedNetGroup=*/fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
            /*prefer_evict=*/fuzzed_data_provider.ConsumeBool(),
            /*m_is_local=*/fuzzed_data_provider.ConsumeBool(),
            /*m_network=*/fuzzed_data_provider.PickValueInArray(ALL_NETWORKS),
            /*m_noban=*/fuzzed_data_provider.ConsumeBool(),
            /*m_conn_type=*/fuzzed_data_provider.PickValueInArray(ALL_CONNECTION_TYPES),
        });
    }
    // Make a copy since eviction_candidates may be in some valid but otherwise
    // indeterminate state after the SelectNodeToEvict(&&) call.
    const std::vector<NodeEvictionCandidate> eviction_candidates_copy = eviction_candidates;
    const std::optional<NodeId> node_to_evict = SelectNodeToEvict(std::move(eviction_candidates));
    const bool has_eligible_candidate{
        std::any_of(eviction_candidates_copy.begin(), eviction_candidates_copy.end(), EvictionEligible)};
    if (!has_eligible_candidate) {
        assert(!node_to_evict);
    }
    if (node_to_evict) {
        const auto it{std::find_if(eviction_candidates_copy.begin(), eviction_candidates_copy.end(),
                                   [&node_to_evict](const NodeEvictionCandidate& eviction_candidate) {
                                       return *node_to_evict == eviction_candidate.id;
                                   })};
        assert(it != eviction_candidates_copy.end());
        assert(!it->m_noban);
        assert(it->m_conn_type == ConnectionType::INBOUND);
    }

    std::vector<NodeEvictionCandidate> with_protected_noise{eviction_candidates_copy};
    AppendProtectedNoise(with_protected_noise);
    assert(SelectNodeToEvict(std::move(with_protected_noise)) == node_to_evict);
}
