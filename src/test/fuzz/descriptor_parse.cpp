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

//! The converter of mocked descriptors, needs to be initialized when the target is.
MockedDescriptorConverter MOCKED_DESC_CONVERTER;

/** A serialized descriptor must be accepted by the strict parser unchanged. */
static void AssertDescriptorRoundTrip(const std::string& serialized)
{
    FlatSigningProvider roundtrip_provider;
    std::string error;
    const auto reparsed = Parse(serialized, roundtrip_provider, error, /*require_checksum=*/true);
    assert(!reparsed.empty());
    bool found_match{false};
    for (const auto& descriptor : reparsed) {
        assert(descriptor);
        found_match = found_match || descriptor->ToString() == serialized;
    }
    assert(found_match);
}

/** A private descriptor must preserve its private serialization after parsing. */
static void AssertPrivateDescriptorRoundTrip(const std::string& serialized)
{
    FlatSigningProvider roundtrip_provider;
    std::string error;
    const auto reparsed = Parse(serialized, roundtrip_provider, error, /*require_checksum=*/true);
    assert(!reparsed.empty());
    bool found_match{false};
    for (const auto& descriptor : reparsed) {
        assert(descriptor);
        std::string reparsed_private;
        (void)descriptor->ToPrivateString(roundtrip_provider, reparsed_private);
        found_match = found_match || reparsed_private == serialized;
    }
    assert(found_match);
}

/** Test a successfully parsed descriptor. */
static void TestDescriptor(const Descriptor& desc, FlatSigningProvider& sig_provider, std::optional<bool>& is_ranged, std::optional<bool>& is_solvable)
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

    // Serialization to string representation. Every successful representation
    // must remain a valid, checksummed descriptor when parsed independently.
    const std::string public_string{desc.ToString()};
    AssertDescriptorRoundTrip(public_string);

    std::string private_string;
    const bool has_private_key{desc.ToPrivateString(sig_provider, private_string)};
    assert(!private_string.empty());
    AssertPrivateDescriptorRoundTrip(private_string);
    if (!has_private_key) assert(private_string == public_string);

    std::string normalized_string;
    if (desc.ToNormalizedString(sig_provider, normalized_string)) {
        AssertDescriptorRoundTrip(normalized_string);
    }

    // Serialization to Script.
    DescriptorCache cache;
    std::vector<CScript> out_scripts;
    (void)desc.Expand(0, sig_provider, out_scripts, sig_provider, &cache);
    (void)desc.ExpandPrivate(0, sig_provider, sig_provider);
    (void)desc.ExpandFromCache(0, cache, out_scripts, sig_provider);

    // If we could serialize to script, InferDescriptor must return a non-ranged
    // descriptor that expands back to exactly the same script.
    if (!out_scripts.empty()) {
        for (const CScript& script : out_scripts) {
            const auto inferred{InferDescriptor(script, sig_provider)};
            assert(inferred);
            assert(!inferred->IsRange());
            FlatSigningProvider inferred_provider;
            std::vector<CScript> inferred_scripts;
            assert(inferred->Expand(0, inferred_provider, inferred_scripts, inferred_provider));
            assert(inferred_scripts.size() == 1);
            assert(inferred_scripts.front() == script);
        }

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
        FlatSigningProvider signing_provider;
        std::string error;
        const auto desc = Parse(*descriptor, signing_provider, error);
        std::optional<bool> is_ranged;
        std::optional<bool> is_solvable;
        for (const auto& d : desc) {
            assert(d);
            TestDescriptor(*d, signing_provider, is_ranged, is_solvable);
        }
    }
}

FUZZ_TARGET(descriptor_parse, .init = initialize_descriptor_parse)
{
    if (IsTooExpensive(buffer)) return;

    const std::string descriptor(buffer.begin(), buffer.end());
    FlatSigningProvider signing_provider;
    std::string error;
    for (const bool require_checksum : {true, false}) {
        const auto desc = Parse(descriptor, signing_provider, error, require_checksum);
        std::optional<bool> is_ranged;
        std::optional<bool> is_solvable;
        for (const auto& d : desc) {
            assert(d);
            TestDescriptor(*d, signing_provider, is_ranged, is_solvable);
        }
    }
}
