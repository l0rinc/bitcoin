// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>

#include <addrdb.h>
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
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

int32_t GetCheckRatio()
{
    return std::clamp<int32_t>(g_setup->m_node.args->GetIntArg("-checkaddrman", 0), 0, 1000000);
}

using AddrInfoByService = std::unordered_map<CService, AddrInfo, CServiceHash>;
static constexpr size_t MAX_POSITION_LOOKUPS{16};
static constexpr size_t MAX_GET_ADDR_ORACLE{256};
static constexpr size_t MAX_INTERNAL_CHECK_SIZE{4096};

void AddUniqueAddrInfo(AddrInfoByService& unique, const AddrInfo& info)
{
    const CService& service{info};
    const auto [it, inserted]{unique.emplace(service, info)};
    if (!inserted) {
        const AddrInfo& previous{it->second};
        assert(previous.source == info.source);
        assert(previous.nServices == info.nServices);
        assert(previous.nTime == info.nTime);
        assert(previous.m_last_try == info.m_last_try);
        assert(previous.m_last_count_attempt == info.m_last_count_attempt);
        assert(previous.m_last_success == info.m_last_success);
        assert(previous.nAttempts == info.nAttempts);
        assert(previous.nRefCount == info.nRefCount);
        assert(previous.fInTried == info.fInTried);
    }
}

void AssertGetAddrResult(const std::vector<CAddress>& actual, const AddrInfoByService& new_unique, const AddrInfoByService& tried_unique, std::optional<Network> network, std::optional<size_t> max_addresses)
{
    std::unordered_set<CService, CServiceHash> actual_services;
    for (const CAddress& address : actual) {
        assert(address.IsValid());
        if (network) assert(address.GetNetClass() == *network);
        assert(actual_services.insert(static_cast<const CService&>(address)).second);
    }

    if (max_addresses) {
        assert(actual_services.size() <= *max_addresses);
        for (const CService& service : actual_services) {
            const auto new_it{new_unique.find(service)};
            const auto tried_it{tried_unique.find(service)};
            assert(new_it != new_unique.end() || tried_it != tried_unique.end());
            const AddrInfo& info{new_it != new_unique.end() ? new_it->second : tried_it->second};
            if (network) assert(info.GetNetClass() == *network);
        }
    } else {
        std::unordered_set<CService, CServiceHash> expected_services;
        expected_services.reserve(new_unique.size() + tried_unique.size());
        for (const auto& [service, info] : new_unique) {
            if (!network || info.GetNetClass() == *network) expected_services.insert(service);
        }
        for (const auto& [service, info] : tried_unique) {
            if (!network || info.GetNetClass() == *network) expected_services.insert(service);
        }
        assert(actual_services == expected_services);
    }
}

void AssertAddrmanContracts(AddrMan& addrman)
{
    const auto new_entries{addrman.GetEntries(/*from_tried=*/false)};
    const auto tried_entries{addrman.GetEntries(/*from_tried=*/true)};
    AddrInfoByService new_unique;
    AddrInfoByService tried_unique;

    for (const auto& [info, position] : new_entries) {
        assert(!info.fInTried);
        assert(!position.tried);
        assert(position.multiplicity == info.nRefCount);
        assert(position.multiplicity >= 1);
        assert(position.multiplicity <= ADDRMAN_NEW_BUCKETS_PER_ADDRESS);
        assert(position.bucket >= 0 && position.bucket < ADDRMAN_NEW_BUCKET_COUNT);
        assert(position.position >= 0 && position.position < ADDRMAN_BUCKET_SIZE);
        AddUniqueAddrInfo(new_unique, info);
    }
    for (const auto& [info, position] : tried_entries) {
        assert(info.fInTried);
        assert(position.tried);
        assert(position.multiplicity == 1);
        assert(info.nRefCount == 0);
        assert(position.bucket >= 0 && position.bucket < ADDRMAN_TRIED_BUCKET_COUNT);
        assert(position.position >= 0 && position.position < ADDRMAN_BUCKET_SIZE);
        AddUniqueAddrInfo(tried_unique, info);
    }

    size_t tried_position_lookups{0};
    for (const auto& [service, info] : tried_unique) {
        if (tried_position_lookups++ == MAX_POSITION_LOOKUPS) break;
        assert(!new_unique.contains(service));
        const auto position{addrman.FindAddressEntry(CAddress{service, info.nServices})};
        assert(position);
        assert(position->tried);
        assert(position->multiplicity == 1);
    }
    size_t new_position_lookups{0};
    for (const auto& [service, info] : new_unique) {
        if (new_position_lookups++ == MAX_POSITION_LOOKUPS) break;
        const auto position{addrman.FindAddressEntry(CAddress{service, info.nServices})};
        assert(position);
        assert(!position->tried);
        assert(position->multiplicity == info.nRefCount);
    }

    assert(addrman.Size(std::nullopt, /*in_new=*/true) == new_unique.size());
    assert(addrman.Size(std::nullopt, /*in_new=*/false) == tried_unique.size());
    assert(addrman.Size() == new_unique.size() + tried_unique.size());

    std::unordered_map<Network, size_t> new_counts;
    std::unordered_map<Network, size_t> tried_counts;
    std::unordered_set<Network> net_classes;
    for (const auto& [service, info] : new_unique) {
        ++new_counts[info.GetNetwork()];
        net_classes.insert(info.GetNetClass());
    }
    for (const auto& [service, info] : tried_unique) {
        ++tried_counts[info.GetNetwork()];
        net_classes.insert(info.GetNetClass());
    }

    std::optional<size_t> get_addr_limit;
    if (new_unique.size() + tried_unique.size() > MAX_GET_ADDR_ORACLE) get_addr_limit = MAX_GET_ADDR_ORACLE;
    AssertGetAddrResult(addrman.GetAddr(get_addr_limit.value_or(0), /*max_pct=*/0, std::nullopt, /*filtered=*/false), new_unique, tried_unique, std::nullopt, get_addr_limit);

    const auto selected_new{addrman.Select(/*new_only=*/true)};
    if (new_unique.empty()) {
        assert(!selected_new.first.IsValid());
    } else {
        assert(new_unique.contains(static_cast<const CService&>(selected_new.first)));
    }
    const auto selected_any{addrman.Select(/*new_only=*/false)};
    if (new_unique.empty() && tried_unique.empty()) {
        assert(!selected_any.first.IsValid());
    } else {
        const CService& service{selected_any.first};
        assert(new_unique.contains(service) || tried_unique.contains(service));
    }

    for (const Network network : ALL_NETWORKS) {
        const std::unordered_set<Network> networks{network};
        const auto selected{addrman.Select(/*new_only=*/false, networks)};
        const bool have_network{new_counts.contains(network) || tried_counts.contains(network)};
        const bool have_net_class{net_classes.contains(network)};
        if (have_network) {
            assert(selected.first.IsValid());
            assert(selected.first.GetNetwork() == network);
        } else {
            assert(!selected.first.IsValid());
        }
        if (!get_addr_limit || have_net_class) {
            AssertGetAddrResult(addrman.GetAddr(get_addr_limit.value_or(0), /*max_pct=*/0, network, /*filtered=*/false), new_unique, tried_unique, network, get_addr_limit);
        }
    }

    for (const Network network : ALL_NETWORKS) {
        const size_t new_count{new_counts[network]};
        const size_t tried_count{tried_counts[network]};
        assert(addrman.Size(network, /*in_new=*/true) == new_count);
        assert(addrman.Size(network, /*in_new=*/false) == tried_count);
        assert(addrman.Size(network) == new_count + tried_count);
    }
}

void AssertAddrmanContracts(AddrManDeterministic& addrman, bool check_internal = true)
{
    AssertAddrmanContracts(static_cast<AddrMan&>(addrman));
    if (check_internal && addrman.Size() <= MAX_INTERNAL_CHECK_SIZE) assert(addrman.CheckConsistency() == 0);
}

std::optional<AddrInfo> FindAddrInfo(const AddrManDeterministic& addrman, const CService& service)
{
    return addrman.GetInfo(service);
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
    bool read_succeeded{true};
    try {
        ReadFromStream(addr_man, data_stream);
    } catch (const std::exception&) {
        read_succeeded = false;
    }
    if (read_succeeded) AssertAddrmanContracts(addr_man);
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
    AssertAddrmanContracts(addr_man);
    size_t operations{0};
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
                const auto before{FindAddrInfo(addr_man, addr)};
                const bool moved{addr_man.Good(addr, time)};
                const auto after{FindAddrInfo(addr_man, addr)};
                if (!before) {
                    assert(!moved);
                    assert(!after);
                } else {
                    assert(after);
                    assert(after->m_last_success == time);
                    assert(after->m_last_try == time);
                    assert(after->nAttempts == 0);
                    if (moved) assert(after->fInTried);
                }
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto count_failure = fuzzed_data_provider.ConsumeBool();
                auto time = ConsumeTime(fuzzed_data_provider);
                const auto before{FindAddrInfo(addr_man, addr)};
                addr_man.Attempt(addr, count_failure, time);
                const auto after{FindAddrInfo(addr_man, addr)};
                if (before) {
                    assert(after);
                    assert(after->m_last_try == time);
                    assert(after->nAttempts >= before->nAttempts);
                } else {
                    assert(!after);
                }
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto time = ConsumeTime(fuzzed_data_provider);
                const auto before{FindAddrInfo(addr_man, addr)};
                addr_man.Connected(addr, time);
                const auto after{FindAddrInfo(addr_man, addr)};
                if (before) {
                    assert(after);
                    if (time - before->nTime > std::chrono::minutes{20}) {
                        assert(after->nTime == time);
                    } else {
                        assert(after->nTime == before->nTime);
                    }
                } else {
                    assert(!after);
                }
            },
            [&] {
                auto addr = ConsumeService(fuzzed_data_provider);
                auto n_services = ConsumeWeakEnum(fuzzed_data_provider, ALL_SERVICE_FLAGS);
                const auto before{FindAddrInfo(addr_man, addr)};
                addr_man.SetServices(addr, n_services);
                const auto after{FindAddrInfo(addr_man, addr)};
                if (before) {
                    assert(after);
                    assert(after->nServices == n_services);
                } else {
                    assert(!after);
                }
            });
        // The exhaustive oracle scans every table slot; operation-local assertions above stay on
        // every transition, while the cross-index oracle runs often enough to catch drift.
        ++operations;
        const size_t full_check_interval{addr_man.Size() > 4096 ? size_t{1024} : size_t{64}};
        if (operations % full_check_interval == 0) {
            AssertAddrmanContracts(addr_man, operations % (full_check_interval * 4) == 0);
        }
    }
    const AddrMan& const_addr_man{addr_man};
    std::optional<Network> network;
    if (fuzzed_data_provider.ConsumeBool()) {
        network = fuzzed_data_provider.PickValueInArray(ALL_NETWORKS);
    }
    auto max_addresses = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 4096);
    auto max_pct = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 100);
    auto filtered = fuzzed_data_provider.ConsumeBool();
    (void)const_addr_man.GetAddr(max_addresses, max_pct, network, filtered);

    std::unordered_set<Network> nets;
    for (const auto& net : ALL_NETWORKS) {
        if (fuzzed_data_provider.ConsumeBool()) {
            nets.insert(net);
        }
    }
    (void)const_addr_man.Select(fuzzed_data_provider.ConsumeBool(), nets);

    std::optional<bool> in_new;
    if (fuzzed_data_provider.ConsumeBool()) {
        in_new = fuzzed_data_provider.ConsumeBool();
    }
    (void)const_addr_man.Size(network, in_new);
    AssertAddrmanContracts(addr_man);
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
    AssertAddrmanContracts(addr_man1);
    data_stream << addr_man1;
    data_stream >> addr_man2;
    AssertAddrmanContracts(addr_man2);
    assert(addr_man1 == addr_man2);
}
