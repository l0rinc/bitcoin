// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <addrman.h>
#include <addrman_impl.h>
#include <chainparams.h>
#include <common/args.h>
#include <merkleblock.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <util/asmap.h>
#include <util/chaintype.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <ctime>
#include <ios>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

int32_t GetCheckRatio()
{
    return std::clamp<int32_t>(g_setup->m_node.args->GetIntArg("-checkaddrman", 0), 0, 1000000);
}
} // namespace

void initialize_addrman()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::REGTEST);
    g_setup = testing_setup.get();
}

FUZZ_TARGET(data_stream_addr_man, .init = initialize_addrman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    DataStream data_stream = ConsumeDataStream(fuzzed_data_provider);
    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    AddrMan addr_man(netgroupman, /*deterministic=*/false, GetCheckRatio());
    try {
        ReadFromStream(addr_man, data_stream);
    } catch (const InvalidAddrManVersionError&) {
    } catch (const std::ios_base::failure&) {
    } catch (const std::runtime_error& e) {
        assert(std::string_view{e.what()} == "Invalid network magic number");
    }
}

/**
 * Generate a random address. Always returns a valid address.
 */
CNetAddr RandAddr(FuzzedDataProvider& fuzzed_data_provider, FastRandomContext& fast_random_context)
{
    CNetAddr addr;
    assert(!addr.IsValid());
    for (size_t i = 0; i < 8 && !addr.IsValid(); ++i) {
        if (fuzzed_data_provider.remaining_bytes() > 1 && fuzzed_data_provider.ConsumeBool()) {
            addr = ConsumeNetAddr(fuzzed_data_provider);
        } else {
            addr = ConsumeNetAddr(fuzzed_data_provider, &fast_random_context);
        }
    }

    // Return a dummy IPv4 5.5.5.5 if we generated an invalid address.
    if (!addr.IsValid()) {
        in_addr v4_addr = {};
        v4_addr.s_addr = 0x05050505;
        addr = CNetAddr{v4_addr};
    }

    return addr;
}

/** Fill addrman with lots of addresses from lots of sources.  */
void FillAddrman(AddrMan& addrman, FuzzedDataProvider& fuzzed_data_provider)
{
    // Add a fraction of the addresses to the "tried" table.
    // 0, 1, 2, 3 corresponding to 0%, 100%, 50%, 33%
    const size_t n = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 3);

    const size_t num_sources = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 50);
    CNetAddr prev_source;
    // Generate a FastRandomContext seed to use inside the loops instead of
    // fuzzed_data_provider. When fuzzed_data_provider is exhausted it
    // just returns 0.
    FastRandomContext fast_random_context{ConsumeUInt256(fuzzed_data_provider)};
    for (size_t i = 0; i < num_sources; ++i) {
        const auto source = RandAddr(fuzzed_data_provider, fast_random_context);
        const size_t num_addresses = fast_random_context.randrange(500) + 1; // [1..500]

        for (size_t j = 0; j < num_addresses; ++j) {
            const auto addr = CAddress{CService{RandAddr(fuzzed_data_provider, fast_random_context), 8333}, NODE_NETWORK};
            const std::chrono::seconds time_penalty{fast_random_context.randrange(100000001)};
            addrman.Add({addr}, source, time_penalty);

            if (n > 0 && addrman.Size() % n == 0) {
                addrman.Good(addr, Now<NodeSeconds>());
            }

            // Add 10% of the addresses from more than one source.
            if (fast_random_context.randrange(10) == 0 && prev_source.IsValid()) {
                addrman.Add({addr}, prev_source, time_penalty);
            }
        }
        prev_source = source;
    }
}

void AssertEntriesMatchSizes(const AddrMan& addrman)
{
    struct EntryStats {
        size_t new_occurrences{0};
        size_t tried_occurrences{0};
        Network network{NET_UNROUTABLE};
    };

    const auto new_entries{addrman.GetEntries(/*from_tried=*/false)};
    const auto tried_entries{addrman.GetEntries(/*from_tried=*/true)};
    std::map<CService, EntryStats> stats;
    std::map<Network, std::pair<size_t, size_t>> network_counts;

    for (const auto& [info, position] : new_entries) {
        assert(!position.tried);
        assert(!info.fInTried);
        assert(info.IsValid());
        assert(position.bucket >= 0 && position.bucket < ADDRMAN_NEW_BUCKET_COUNT);
        assert(position.position >= 0 && position.position < ADDRMAN_BUCKET_SIZE);
        assert(position.multiplicity == info.nRefCount);
        assert(position.multiplicity >= 1);
        assert(position.multiplicity <= ADDRMAN_NEW_BUCKETS_PER_ADDRESS);
        auto& entry_stats{stats[static_cast<const CService&>(info)]};
        ++entry_stats.new_occurrences;
        entry_stats.network = info.GetNetwork();
    }

    for (const auto& [info, position] : tried_entries) {
        assert(position.tried);
        assert(info.fInTried);
        assert(info.IsValid());
        assert(position.bucket >= 0 && position.bucket < ADDRMAN_TRIED_BUCKET_COUNT);
        assert(position.position >= 0 && position.position < ADDRMAN_BUCKET_SIZE);
        assert(position.multiplicity == 1);
        auto& entry_stats{stats[static_cast<const CService&>(info)]};
        ++entry_stats.tried_occurrences;
        entry_stats.network = info.GetNetwork();
    }

    size_t new_unique{0};
    size_t tried_unique{0};
    for (const auto& [service, entry_stats] : stats) {
        assert(entry_stats.new_occurrences == 0 || entry_stats.tried_occurrences == 0);
        if (entry_stats.new_occurrences > 0) {
            ++new_unique;
            ++network_counts[entry_stats.network].first;
        }
        if (entry_stats.tried_occurrences > 0) {
            ++tried_unique;
            ++network_counts[entry_stats.network].second;
        }
    }

    for (const auto& [info, position] : new_entries) {
        assert(stats.at(static_cast<const CService&>(info)).new_occurrences == static_cast<size_t>(position.multiplicity));
    }
    for (const auto& [info, _] : tried_entries) {
        assert(stats.at(static_cast<const CService&>(info)).tried_occurrences == 1);
    }

    assert(new_unique == addrman.Size(/*net=*/std::nullopt, /*in_new=*/true));
    assert(tried_unique == addrman.Size(/*net=*/std::nullopt, /*in_new=*/false));
    assert(new_unique + tried_unique == addrman.Size());
    for (const auto network : ALL_NETWORKS) {
        assert(network_counts[network].first == addrman.Size(network, /*in_new=*/true));
        assert(network_counts[network].second == addrman.Size(network, /*in_new=*/false));
    }
}

FUZZ_TARGET(addrman, .init = initialize_addrman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};
    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    auto addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio());
    if (fuzzed_data_provider.ConsumeBool()) {
        const std::vector<uint8_t> serialized_data{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        DataStream ds{serialized_data};
        try {
            ds >> *addr_man_ptr;
        } catch (const std::ios_base::failure&) {
            addr_man_ptr = std::make_unique<AddrManDeterministic>(netgroupman, fuzzed_data_provider, GetCheckRatio());
        }
    }
    AddrManDeterministic& addr_man = *addr_man_ptr;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                addr_man.ResolveCollisions();
            },
            [&] {
                (void)addr_man.SelectTriedCollision();
            },
            [&] {
                std::vector<CAddress> addresses;
                LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
                    addresses.push_back(ConsumeAddress(fuzzed_data_provider));
                }
                auto net_addr = ConsumeNetAddr(fuzzed_data_provider);
                auto time_penalty = ConsumeDuration<std::chrono::seconds>(fuzzed_data_provider, /*min=*/0s, /*max=*/100000000s);
                addr_man.Add(addresses, net_addr, time_penalty);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto time = ConsumeTime(fuzzed_data_provider);
                addr_man.Good(addr, time);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto count_failure = fuzzed_data_provider.ConsumeBool();
                auto time = ConsumeTime(fuzzed_data_provider);
                addr_man.Attempt(addr, count_failure, time);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto time = ConsumeTime(fuzzed_data_provider);
                addr_man.Connected(addr, time);
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto n_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
                addr_man.SetServices(addr, n_services);
            });
    }
    const AddrMan& const_addr_man{addr_man};
    AssertEntriesMatchSizes(const_addr_man);
    std::optional<Network> network;
    if (fuzzed_data_provider.ConsumeBool()) {
        network = fuzzed_data_provider.PickValueInArray(ALL_NETWORKS);
    }
    auto max_addresses = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100);
    auto filtered = fuzzed_data_provider.ConsumeBool();
    const size_t max_pct_count{max_pct == 0 ? const_addr_man.Size() : (max_pct * const_addr_man.Size()) / 100};
    const size_t max_count{max_addresses == 0 ? max_pct_count : std::min(max_pct_count, max_addresses)};
    const auto addresses{const_addr_man.GetAddr(max_addresses, max_pct, network, filtered)};
    assert(addresses.size() <= max_count);
    for (const auto& addr : addresses) {
        assert(addr.IsValid());
        if (network) assert(addr.GetNetClass() == *network);
    }

    std::unordered_set<Network> nets;
    for (const auto& net : ALL_NETWORKS) {
        if (fuzzed_data_provider.ConsumeBool()) {
            nets.insert(net);
        }
    }
    const bool new_only{fuzzed_data_provider.ConsumeBool()};
    const auto eligible_select_count{[&] {
        if (nets.empty()) {
            return new_only ? const_addr_man.Size(/*net=*/std::nullopt, /*in_new=*/true) : const_addr_man.Size();
        }
        size_t count{0};
        for (const auto net : nets) {
            count += const_addr_man.Size(net, /*in_new=*/true);
            if (!new_only) count += const_addr_man.Size(net, /*in_new=*/false);
        }
        return count;
    }()};
    const auto selected{const_addr_man.Select(new_only, nets)};
    if (selected.first.IsValid()) {
        if (!nets.empty()) assert(nets.contains(selected.first.GetNetClass()));
        if (new_only) {
            const auto new_entries{const_addr_man.GetEntries(/*from_tried=*/false)};
            assert(std::any_of(new_entries.begin(), new_entries.end(), [&](const auto& entry) {
                return static_cast<const CService&>(entry.first) == static_cast<const CService&>(selected.first);
            }));
        }
        assert(eligible_select_count > 0);
    } else {
        assert(eligible_select_count == 0);
    }

    std::optional<bool> in_new;
    if (fuzzed_data_provider.ConsumeBool()) {
        in_new = fuzzed_data_provider.ConsumeBool();
    }
    (void)const_addr_man.Size(network, in_new);
    DataStream data_stream{};
    data_stream << const_addr_man;
}

// Check that serialize followed by unserialize produces the same addrman.
FUZZ_TARGET(addrman_serdeser, .init = initialize_addrman)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider)};

    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    AddrManDeterministic addr_man1{netgroupman, fuzzed_data_provider, GetCheckRatio()};
    AddrManDeterministic addr_man2{netgroupman, fuzzed_data_provider, GetCheckRatio()};

    DataStream data_stream{};

    FillAddrman(addr_man1, fuzzed_data_provider);
    data_stream << addr_man1;
    data_stream >> addr_man2;
    assert(addr_man1 == addr_man2);
}
