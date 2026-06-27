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

FUZZ_TARGET(blockfilter)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
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
