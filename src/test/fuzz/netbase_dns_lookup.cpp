// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <netaddress.h>
#include <netbase.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/net.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
std::vector<CNetAddr> FilterLookupResults(const std::vector<CNetAddr>& resolved_addresses, unsigned int max_results)
{
    std::vector<CNetAddr> expected;
    for (const CNetAddr& resolved_address : resolved_addresses) {
        if (max_results > 0 && expected.size() >= max_results) {
            break;
        }
        if (!resolved_address.IsInternal()) {
            expected.push_back(resolved_address);
        }
    }
    return expected;
}

void AssertAddressesEqual(const std::vector<CNetAddr>& actual, const std::vector<CNetAddr>& expected)
{
    assert(actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        assert(actual[i] == expected[i]);
    }
}

std::string HostNameForLookupHost(const std::string& name)
{
    if (name.size() >= 2 && name.front() == '[' && name.back() == ']') {
        return name.substr(1, name.size() - 2);
    }
    return name;
}

void AssertSpecialOrEmpty(const std::vector<CNetAddr>& actual, const std::string& host, unsigned int max_results, bool callback_called)
{
    if (callback_called) return;

    CNetAddr special;
    if (special.SetSpecial(host)) {
        assert(actual.size() == 1);
        assert(actual.front() == special);
    } else {
        assert(actual.empty());
    }
    assert(actual.size() <= max_results || max_results == 0);
}

void AssertSpecialOrEmpty(const std::vector<CService>& actual, const std::string& host, unsigned int max_results, uint16_t port, bool callback_called)
{
    if (callback_called) return;

    CNetAddr special;
    if (special.SetSpecial(host)) {
        assert(actual.size() == 1);
        assert(static_cast<const CNetAddr&>(actual.front()) == special);
        assert(actual.front().GetPort() == port);
    } else {
        assert(actual.empty());
    }
    assert(actual.size() <= max_results || max_results == 0);
}

void AssertServicesMatch(const std::vector<CService>& actual, const std::vector<CNetAddr>& expected, uint16_t port)
{
    assert(actual.size() == expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        assert(static_cast<const CNetAddr&>(actual[i]) == expected[i]);
        assert(actual[i].GetPort() == port);
    }
}
} // namespace

FUZZ_TARGET(netbase_dns_lookup)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::string name = fuzzed_data_provider.ConsumeRandomLengthString(512);
    const unsigned int max_results = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
    const bool allow_lookup = fuzzed_data_provider.ConsumeBool();
    const uint16_t default_port = fuzzed_data_provider.ConsumeIntegral<uint16_t>();

    bool dns_lookup_called{false};
    std::string dns_lookup_name;
    bool dns_lookup_allowed{false};
    std::vector<CNetAddr> dns_lookup_results;
    auto reset_dns_lookup_state = [&] {
        dns_lookup_called = false;
        dns_lookup_name.clear();
        dns_lookup_allowed = false;
        dns_lookup_results.clear();
    };
    auto fuzzed_dns_lookup_function = [&](const std::string& lookup_name, bool lookup_allowed) {
        dns_lookup_called = true;
        dns_lookup_name = lookup_name;
        dns_lookup_allowed = lookup_allowed;
        dns_lookup_results.clear();
        LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
            dns_lookup_results.push_back(ConsumeNetAddr(fuzzed_data_provider));
        }
        return dns_lookup_results;
    };

    {
        reset_dns_lookup_state();
        const std::vector<CNetAddr> resolved_addresses{LookupHost(name, max_results, allow_lookup, fuzzed_dns_lookup_function)};
        for (const CNetAddr& resolved_address : resolved_addresses) {
            assert(!resolved_address.IsInternal());
        }
        assert(resolved_addresses.size() <= max_results || max_results == 0);
        if (dns_lookup_called) {
            assert(dns_lookup_name == HostNameForLookupHost(name));
            assert(dns_lookup_allowed == allow_lookup);
            AssertAddressesEqual(resolved_addresses, FilterLookupResults(dns_lookup_results, max_results));
        } else {
            AssertSpecialOrEmpty(resolved_addresses, HostNameForLookupHost(name), max_results, dns_lookup_called);
        }
    }
    {
        reset_dns_lookup_state();
        const std::optional<CNetAddr> resolved_address{LookupHost(name, allow_lookup, fuzzed_dns_lookup_function)};
        if (resolved_address.has_value()) {
            assert(!resolved_address.value().IsInternal());
        }
        if (dns_lookup_called) {
            assert(dns_lookup_name == HostNameForLookupHost(name));
            assert(dns_lookup_allowed == allow_lookup);
            const std::vector<CNetAddr> expected{FilterLookupResults(dns_lookup_results, 1)};
            assert(resolved_address.has_value() == !expected.empty());
            if (!expected.empty()) {
                assert(resolved_address.value() == expected.front());
            }
        } else {
            CNetAddr special;
            if (special.SetSpecial(HostNameForLookupHost(name))) {
                assert(resolved_address.has_value());
                assert(resolved_address.value() == special);
            } else {
                assert(!resolved_address.has_value());
            }
        }
    }
    {
        reset_dns_lookup_state();
        uint16_t expected_port{default_port};
        std::string expected_host;
        SplitHostPort(name, expected_port, expected_host);
        const std::vector<CService> resolved_services{Lookup(name, default_port, allow_lookup, max_results, fuzzed_dns_lookup_function)};
        for (const CNetAddr& resolved_service : resolved_services) {
            assert(!resolved_service.IsInternal());
        }
        assert(resolved_services.size() <= max_results || max_results == 0);
        if (dns_lookup_called) {
            assert(dns_lookup_name == expected_host);
            assert(dns_lookup_allowed == allow_lookup);
            AssertServicesMatch(resolved_services, FilterLookupResults(dns_lookup_results, max_results), expected_port);
        } else {
            AssertSpecialOrEmpty(resolved_services, expected_host, max_results, expected_port, dns_lookup_called);
        }
    }
    {
        reset_dns_lookup_state();
        uint16_t expected_port{default_port};
        std::string expected_host;
        SplitHostPort(name, expected_port, expected_host);
        const std::optional<CService> resolved_service{Lookup(name, default_port, allow_lookup, fuzzed_dns_lookup_function)};
        if (resolved_service.has_value()) {
            assert(!resolved_service.value().IsInternal());
            assert(resolved_service->GetPort() == expected_port);
        }
        if (dns_lookup_called) {
            assert(dns_lookup_name == expected_host);
            assert(dns_lookup_allowed == allow_lookup);
            const std::vector<CNetAddr> expected{FilterLookupResults(dns_lookup_results, 1)};
            assert(resolved_service.has_value() == !expected.empty());
            if (!expected.empty()) {
                assert(static_cast<const CNetAddr&>(resolved_service.value()) == expected.front());
            }
        } else {
            CNetAddr special;
            if (special.SetSpecial(expected_host)) {
                assert(resolved_service.has_value());
                assert(static_cast<const CNetAddr&>(resolved_service.value()) == special);
            } else {
                assert(!resolved_service.has_value());
            }
        }
    }
    {
        reset_dns_lookup_state();
        uint16_t expected_port{default_port};
        std::string expected_host;
        SplitHostPort(name, expected_port, expected_host);
        CService resolved_service = LookupNumeric(name, default_port, fuzzed_dns_lookup_function);
        assert(!resolved_service.IsInternal());
        if (dns_lookup_called) {
            assert(dns_lookup_name == expected_host);
            assert(!dns_lookup_allowed);
            const std::vector<CNetAddr> expected{FilterLookupResults(dns_lookup_results, 1)};
            if (expected.empty()) {
                assert(!resolved_service.IsValid());
            } else {
                assert(static_cast<const CNetAddr&>(resolved_service) == expected.front());
                assert(resolved_service.GetPort() == expected_port);
            }
        } else {
            CNetAddr special;
            if (special.SetSpecial(expected_host)) {
                assert(static_cast<const CNetAddr&>(resolved_service) == special);
                assert(resolved_service.GetPort() == expected_port);
            } else {
                assert(!resolved_service.IsValid());
            }
        }
    }
    {
        const CSubNet subnet{LookupSubNet(name)};
        if (subnet.IsValid()) {
            const std::string canonical{subnet.ToString()};
            assert(!canonical.empty());
            assert(LookupSubNet(canonical) == subnet);
        }
    }
}
