// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <util/check.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
constexpr size_t MAX_CONSTRUCTED_GCS_ELEMENTS{64};
constexpr size_t MAX_CONSTRUCTED_GCS_ELEMENT_SIZE{128};

void AssertGCSFilterMatchesElements(const GCSFilter& filter, const GCSFilter::Params& params, const GCSFilter::ElementSet& elements)
{
    Assert(filter.GetParams().m_siphash_k0 == params.m_siphash_k0);
    Assert(filter.GetParams().m_siphash_k1 == params.m_siphash_k1);
    Assert(filter.GetParams().m_P == params.m_P);
    Assert(filter.GetParams().m_M == params.m_M);
    Assert(filter.GetN() == elements.size());
    Assert(!filter.GetEncoded().empty());
    Assert(filter.MatchAny(elements) == !elements.empty());
    for (const auto& element : elements) {
        Assert(filter.Match(element));
    }
}

void AssertConstructedGCSFilter(FuzzedDataProvider& fuzzed_data_provider)
{
    const GCSFilter::Params params{
        fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
        fuzzed_data_provider.ConsumeIntegral<uint64_t>(),
        BASIC_FILTER_P,
        BASIC_FILTER_M,
    };
    GCSFilter::ElementSet elements;
    const size_t element_count{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, MAX_CONSTRUCTED_GCS_ELEMENTS)};
    for (size_t i{0}; i < element_count; ++i) {
        elements.insert(ConsumeRandomLengthByteVector(fuzzed_data_provider, MAX_CONSTRUCTED_GCS_ELEMENT_SIZE));
    }

    const GCSFilter constructed{params, elements};
    AssertGCSFilterMatchesElements(constructed, params, elements);

    const GCSFilter checked{params, constructed.GetEncoded(), /*skip_decode_check=*/false};
    Assert(checked.GetEncoded() == constructed.GetEncoded());
    AssertGCSFilterMatchesElements(checked, params, elements);

    const GCSFilter unchecked{params, constructed.GetEncoded(), /*skip_decode_check=*/true};
    Assert(unchecked.GetEncoded() == constructed.GetEncoded());
    AssertGCSFilterMatchesElements(unchecked, params, elements);
}
} // namespace

FUZZ_TARGET(blockfilter)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider constructed_filter_provider(buffer.data(), buffer.size());
    AssertConstructedGCSFilter(constructed_filter_provider);

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<BlockFilter> block_filter = ConsumeDeserializable<BlockFilter>(fuzzed_data_provider);
    if (!block_filter) {
        return;
    }
    {
        (void)block_filter->ComputeHeader(ConsumeUInt256(fuzzed_data_provider));
        (void)block_filter->GetBlockHash();
        (void)block_filter->GetEncodedFilter();
        (void)block_filter->GetHash();
    }
    {
        const BlockFilterType block_filter_type = block_filter->GetFilterType();
        Assert(!BlockFilterTypeName(block_filter_type).empty());
    }
    {
        const auto assert_same_filter{[&](const BlockFilter& reconstructed) {
            Assert(reconstructed.GetFilterType() == block_filter->GetFilterType());
            Assert(reconstructed.GetBlockHash() == block_filter->GetBlockHash());
            Assert(reconstructed.GetEncodedFilter() == block_filter->GetEncodedFilter());
            Assert(reconstructed.GetHash() == block_filter->GetHash());
        }};

        const BlockFilter checked{
            block_filter->GetFilterType(),
            block_filter->GetBlockHash(),
            block_filter->GetEncodedFilter(),
            /*skip_decode_check=*/false};
        assert_same_filter(checked);

        const BlockFilter unchecked{
            block_filter->GetFilterType(),
            block_filter->GetBlockHash(),
            block_filter->GetEncodedFilter(),
            /*skip_decode_check=*/true};
        assert_same_filter(unchecked);
    }
    {
        DataStream serialized{};
        serialized << *block_filter;
        const std::vector<DataStream::value_type> serialized_bytes{serialized.begin(), serialized.end()};

        DataStream roundtrip_stream{serialized_bytes};
        BlockFilter roundtripped;
        roundtrip_stream >> roundtripped;
        Assert(roundtrip_stream.empty());

        Assert(roundtripped.GetFilterType() == block_filter->GetFilterType());
        Assert(roundtripped.GetBlockHash() == block_filter->GetBlockHash());
        Assert(roundtripped.GetEncodedFilter() == block_filter->GetEncodedFilter());
        Assert(roundtripped.GetHash() == block_filter->GetHash());

        DataStream reserialized{};
        reserialized << roundtripped;
        const std::vector<DataStream::value_type> reserialized_bytes{reserialized.begin(), reserialized.end()};
        Assert(reserialized_bytes == serialized_bytes);
    }
    {
        const GCSFilter gcs_filter = block_filter->GetFilter();
        (void)gcs_filter.GetN();
        (void)gcs_filter.GetParams();
        (void)gcs_filter.GetEncoded();
        Assert(!gcs_filter.MatchAny({}));

        const auto element{ConsumeRandomLengthByteVector(fuzzed_data_provider)};
        Assert(gcs_filter.MatchAny({element}) == gcs_filter.Match(element));

        GCSFilter::ElementSet element_set;
        LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30000) {
            element_set.insert(ConsumeRandomLengthByteVector(fuzzed_data_provider));
        }
        const bool match_any{gcs_filter.MatchAny(element_set)};
        const bool any_match{std::ranges::any_of(element_set, [&](const auto& element) {
            return gcs_filter.Match(element);
        })};
        Assert(match_any == any_match);
    }
}
