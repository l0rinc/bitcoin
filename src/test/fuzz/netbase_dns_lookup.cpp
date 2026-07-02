// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/net.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

FUZZ_TARGET(netbase_dns_lookup)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::string name = fuzzed_data_provider.ConsumeRandomLengthString(512);
    const unsigned int max_results = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const bool allow_lookup = fuzzed_data_provider.ConsumeBool();
    const uint16_t default_port = fuzzed_data_provider.ConsumeIntegral<uint16_t>();

    auto fuzzed_dns_lookup_function = [&](const std::string&, bool) {
        std::vector<CNetAddr> resolved_addresses;
        LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
            resolved_addresses.push_back(ConsumeNetAddr(fuzzed_data_provider));
        }
        return resolved_addresses;
    };

    {
        const std::vector<CNetAddr> resolved_addresses{LookupHost(name, max_results, allow_lookup, fuzzed_dns_lookup_function)};
        for (const CNetAddr& resolved_address : resolved_addresses) {
            assert(!resolved_address.IsInternal());
        }
        assert(resolved_addresses.size() <= max_results || max_results == 0);
    }
    {
        const std::optional<CNetAddr> resolved_address{LookupHost(name, allow_lookup, fuzzed_dns_lookup_function)};
        if (resolved_address.has_value()) {
            assert(!resolved_address.value().IsInternal());
        }
    }
    {
        const std::vector<CService> resolved_services{Lookup(name, default_port, allow_lookup, max_results, fuzzed_dns_lookup_function)};
        for (const CNetAddr& resolved_service : resolved_services) {
            assert(!resolved_service.IsInternal());
        }
        assert(resolved_services.size() <= max_results || max_results == 0);
    }
    {
        const std::optional<CService> resolved_service{Lookup(name, default_port, allow_lookup, fuzzed_dns_lookup_function)};
        if (resolved_service.has_value()) {
            assert(!resolved_service.value().IsInternal());
        }
    }
    {
        CService resolved_service = LookupNumeric(name, default_port, fuzzed_dns_lookup_function);
        assert(!resolved_service.IsInternal());
    }
    {
        const std::string fixed_name{"node.example"};
        const uint16_t fixed_port{fuzzed_data_provider.ConsumeIntegral<uint16_t>()};
        CNetAddr internal_address;
        assert(internal_address.SetInternal("netbase_dns_lookup"));
        const std::optional<CNetAddr> first_lookup{LookupHost("1.2.3.4", /*fAllowLookup=*/false)};
        const std::optional<CNetAddr> second_lookup{LookupHost("5.6.7.8", /*fAllowLookup=*/false)};
        assert(first_lookup && second_lookup);
        const CNetAddr first_address{*first_lookup};
        const CNetAddr second_address{*second_lookup};
        const std::vector<CNetAddr> fixed_addresses{
            internal_address,
            first_address,
            internal_address,
            second_address,
        };
        const bool fixed_allow_lookup{fuzzed_data_provider.ConsumeBool()};
        auto fixed_dns_lookup_function = [&](const std::string& requested_name, bool requested_allow_lookup) {
            assert(requested_name == fixed_name);
            assert(requested_allow_lookup == fixed_allow_lookup);
            return fixed_addresses;
        };

        const std::vector<CNetAddr> one_address{LookupHost(fixed_name, 1, fixed_allow_lookup, fixed_dns_lookup_function)};
        assert((one_address == std::vector<CNetAddr>{first_address}));
        const std::optional<CNetAddr> optional_address{LookupHost(fixed_name, fixed_allow_lookup, fixed_dns_lookup_function)};
        assert(optional_address && *optional_address == first_address);

        const std::vector<CNetAddr> all_addresses{LookupHost(fixed_name, 0, fixed_allow_lookup, fixed_dns_lookup_function)};
        assert((all_addresses == std::vector<CNetAddr>{first_address, second_address}));

        const std::vector<CService> default_port_services{Lookup(fixed_name, fixed_port, fixed_allow_lookup, 0, fixed_dns_lookup_function)};
        assert((default_port_services == std::vector<CService>{CService{first_address, fixed_port}, CService{second_address, fixed_port}}));

        const std::vector<CService> explicit_port_services{Lookup(fixed_name + ":12345", fixed_port, fixed_allow_lookup, 1, fixed_dns_lookup_function)};
        const CService explicit_port_service{first_address, 12345};
        assert((explicit_port_services == std::vector<CService>{explicit_port_service}));
        const std::optional<CService> optional_service{Lookup(fixed_name + ":12345", fixed_port, fixed_allow_lookup, fixed_dns_lookup_function)};
        assert(optional_service && *optional_service == explicit_port_service);
    }
    {
        (void)LookupSubNet(name);
    }
}
