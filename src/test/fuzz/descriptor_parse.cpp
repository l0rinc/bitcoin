// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util/descriptor.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

//! The converter of mocked descriptors, needs to be initialized when the target is.
MockedDescriptorConverter MOCKED_DESC_CONVERTER;
constexpr size_t MAX_CHECKSUM_ORACLE_DESCRIPTOR_SIZE{10'000};

/** Test a successfully parsed descriptor. */
static void TestDescriptor(const Descriptor& desc, FlatSigningProvider& sig_provider, std::string& dummy, std::optional<bool>& is_ranged, std::optional<bool>& is_solvable)
{
    // Trivial helpers.
    (void)desc.IsRange();
    (void)desc.IsSingleType();
    (void)desc.GetOutputType();

    if (is_ranged.has_value()) {
        assert(desc.IsRange() == *is_ranged);
    } else {
        is_ranged = desc.IsRange();
    }
    if (is_solvable.has_value()) {
        assert(desc.IsSolvable() == *is_solvable);
    } else {
        is_solvable = desc.IsSolvable();
    }

    // Serialization to string representation.
    (void)desc.ToString();
    (void)desc.ToPrivateString(sig_provider, dummy);
    (void)desc.ToNormalizedString(sig_provider, dummy);

    // Serialization to Script.
    DescriptorCache cache;
    std::vector<CScript> out_scripts;
    (void)desc.Expand(0, sig_provider, out_scripts, sig_provider, &cache);
    (void)desc.ExpandPrivate(0, sig_provider, sig_provider);
    (void)desc.ExpandFromCache(0, cache, out_scripts, sig_provider);

    // If we could serialize to script we must be able to infer using the same provider.
    if (!out_scripts.empty()) {
        assert(InferDescriptor(out_scripts.back(), sig_provider));

        // The ScriptSize() must match the size of the serialized Script. (ScriptSize() is set for all descs but 'combo()'.)
        const bool is_combo{!desc.IsSingleType()};
        assert(is_combo || desc.ScriptSize() == out_scripts.back().size());
    }

    const auto max_sat_maxsig{desc.MaxSatisfactionWeight(true)};
    const auto max_sat_nonmaxsig{desc.MaxSatisfactionWeight(false)};
    // Whether an estimate is available must not depend on the signature-size
    // assumption, and assuming non-max-size signatures must never increase it.
    assert(max_sat_maxsig.has_value() == max_sat_nonmaxsig.has_value());
    assert(max_sat_nonmaxsig <= max_sat_maxsig);
    const auto max_elems{desc.MaxSatisfactionElems()};
    // We must be able to estimate the max satisfaction size for any solvable descriptor (but combo).
    const bool is_nontop_or_nonsolvable{!*is_solvable || !desc.GetOutputType()};
    const bool is_input_size_info_set{max_sat_maxsig && max_sat_nonmaxsig && max_elems};
    assert(is_input_size_info_set || is_nontop_or_nonsolvable);

    auto max_key_expr = desc.GetMaxKeyExpr();
    auto key_count = desc.GetKeyCount();
    assert((max_key_expr == 0 && key_count == 0) || max_key_expr + 1 == key_count);
}

static std::optional<std::vector<std::string>> ParseAndTestDescriptor(const std::string& descriptor, const bool require_checksum, const bool run_expansion_checks = true)
{
    FlatSigningProvider signing_provider;
    std::string error;
    const auto desc = Parse(descriptor, signing_provider, error, require_checksum);
    if (desc.empty()) return std::nullopt;

    std::optional<bool> is_ranged;
    std::optional<bool> is_solvable;
    std::vector<std::string> descriptor_strings;
    descriptor_strings.reserve(desc.size());
    for (const auto& d : desc) {
        assert(d);
        if (run_expansion_checks) {
            TestDescriptor(*d, signing_provider, error, is_ranged, is_solvable);
        }
        descriptor_strings.push_back(d->ToString());
    }
    return descriptor_strings;
}

void initialize_descriptor_parse()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::MAIN);
}

void initialize_mocked_descriptor_parse()
{
    initialize_descriptor_parse();
    MOCKED_DESC_CONVERTER.Init();
}

FUZZ_TARGET(mocked_descriptor_parse, .init = initialize_mocked_descriptor_parse)
{
    const std::string mocked_descriptor{buffer.begin(), buffer.end()};
    if (const auto descriptor = MOCKED_DESC_CONVERTER.GetDescriptor(mocked_descriptor)) {
        if (IsTooExpensive(MakeUCharSpan(*descriptor))) return;
        (void)ParseAndTestDescriptor(*descriptor, /*require_checksum=*/false);
    }
}

FUZZ_TARGET(descriptor_parse, .init = initialize_descriptor_parse)
{
    if (IsTooExpensive(buffer)) return;

    const std::string descriptor(buffer.begin(), buffer.end());
    const auto without_required_checksum{ParseAndTestDescriptor(descriptor, /*require_checksum=*/false)};
    if (descriptor.size() > MAX_CHECKSUM_ORACLE_DESCRIPTOR_SIZE) return;

    const auto with_required_checksum{ParseAndTestDescriptor(descriptor, /*require_checksum=*/true, /*run_expansion_checks=*/false)};
    if (with_required_checksum) {
        assert(without_required_checksum);
        assert(*without_required_checksum == *with_required_checksum);
    }

    if (without_required_checksum && descriptor.find('#') == std::string::npos) {
        const std::string checksum{GetDescriptorChecksum(descriptor)};
        assert(!checksum.empty());
        const auto with_added_checksum{ParseAndTestDescriptor(descriptor + "#" + checksum, /*require_checksum=*/true, /*run_expansion_checks=*/false)};
        assert(with_added_checksum);
        assert(*without_required_checksum == *with_added_checksum);
    }
}
