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
#include <limits>
#include <optional>
#include <vector>

namespace {

NodeId FindUnusedNodeId(const std::vector<NodeEvictionCandidate>& candidates)
{
    NodeId id{std::numeric_limits<NodeId>::max()};
    while (std::any_of(candidates.begin(), candidates.end(), [id](const auto& candidate) {
        return candidate.id == id;
    })) {
        assert(id > std::numeric_limits<NodeId>::min());
        --id;
    }
    return id;
}

NodeEvictionCandidate MakeProtectedCandidate(const std::vector<NodeEvictionCandidate>& candidates,
                                             ConnectionType connection_type,
                                             bool noban)
{
    NodeEvictionCandidate candidate{};
    candidate.id = FindUnusedNodeId(candidates);
    candidate.m_noban = noban;
    candidate.m_conn_type = connection_type;
    return candidate;
}

void AssertEvictionContracts(const std::vector<NodeEvictionCandidate>& candidates,
                             const std::optional<NodeId>& selected)
{
    const auto is_eligible = [](const NodeEvictionCandidate& candidate) {
        return !candidate.m_noban && candidate.m_conn_type == ConnectionType::INBOUND;
    };
    const bool has_eligible_candidate{std::any_of(candidates.begin(), candidates.end(), is_eligible)};
    if (!has_eligible_candidate) {
        assert(!selected);
    }
    if (selected) {
        assert(std::any_of(candidates.begin(), candidates.end(), [&](const auto& candidate) {
            return candidate.id == *selected && is_eligible(candidate);
        }));
    }

    // Protected candidates are removed before every ranking stage, so adding one with a fresh ID
    // must not change the result for the original candidate set.
    auto with_noban{candidates};
    with_noban.push_back(MakeProtectedCandidate(candidates, ConnectionType::INBOUND, /*noban=*/true));
    assert(SelectNodeToEvict(std::move(with_noban)) == selected);

    auto with_outbound{candidates};
    with_outbound.push_back(MakeProtectedCandidate(candidates, ConnectionType::OUTBOUND_FULL_RELAY, /*noban=*/false));
    assert(SelectNodeToEvict(std::move(with_outbound)) == selected);
}

} // namespace

FUZZ_TARGET(node_eviction)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<NodeEvictionCandidate> eviction_candidates;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        eviction_candidates.push_back({
            /*id=*/fuzzed_data_provider.ConsumeIntegral<NodeId>(),
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
    AssertEvictionContracts(eviction_candidates_copy, node_to_evict);
    if (node_to_evict) {
        assert(std::any_of(eviction_candidates_copy.begin(), eviction_candidates_copy.end(), [&node_to_evict](const NodeEvictionCandidate& eviction_candidate) { return *node_to_evict == eviction_candidate.id; }));
    }
}
