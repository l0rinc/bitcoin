// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <hash.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>

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

    const uint256 previous_header{ConsumeUInt256(fuzzed_data_provider)};
    const uint256 expected_hash{Hash(block_filter->GetEncodedFilter())};
    assert(block_filter->GetHash() == expected_hash);
    assert(block_filter->ComputeHeader(previous_header) == Hash(expected_hash, previous_header));

    DataStream serialized;
    serialized << *block_filter;
    BlockFilter round_tripped;
    serialized >> round_tripped;
    assert(serialized.empty());
    assert(round_tripped.GetFilterType() == block_filter->GetFilterType());
    assert(round_tripped.GetBlockHash() == block_filter->GetBlockHash());
    assert(round_tripped.GetEncodedFilter() == block_filter->GetEncodedFilter());
    assert(round_tripped.GetHash() == expected_hash);
    assert(round_tripped.ComputeHeader(previous_header) == block_filter->ComputeHeader(previous_header));

    {
        const BlockFilterType block_filter_type = block_filter->GetFilterType();
        (void)BlockFilterTypeName(block_filter_type);
    }
    {
        const GCSFilter gcs_filter = block_filter->GetFilter();
        (void)gcs_filter.GetN();
        (void)gcs_filter.GetParams();
        (void)gcs_filter.GetEncoded();
        (void)gcs_filter.Match(ConsumeRandomLengthByteVector(fuzzed_data_provider));
        GCSFilter::ElementSet element_set;
        LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30000) {
            element_set.insert(ConsumeRandomLengthByteVector(fuzzed_data_provider));
        }
        const bool match_any{gcs_filter.MatchAny(element_set)};
        bool match_one{false};
        for (const auto& element : element_set) {
            match_one |= gcs_filter.Match(element);
        }
        assert(match_any == match_one);
    }
}
