// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <chain.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/chaintype.h>
#include <util/check.h>
#include <util/overflow.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

std::optional<arith_uint256> DecodeCompactTarget(unsigned int nBits)
{
    bool negative{false};
    bool overflow{false};
    arith_uint256 target;
    target.SetCompact(nBits, &negative, &overflow);
    if (negative || target == 0 || overflow) return {};
    return target;
}

std::optional<arith_uint256> ReferenceTarget(unsigned int nBits, const uint256& pow_limit)
{
    const auto target{DecodeCompactTarget(nBits)};
    if (!target || *target > UintToArith256(pow_limit)) return {};
    return target;
}

arith_uint256 ReferenceBlockProof(unsigned int nBits)
{
    const auto target{DecodeCompactTarget(nBits)};
    if (!target) return {};
    return (~*target / (*target + 1)) + 1;
}

struct PowState {
    const CBlockIndex* pprev;
    int nHeight;
    int64_t nTime;
    uint32_t nBits;
    arith_uint256 nChainWork;
};

std::vector<PowState> Snapshot(const std::vector<std::unique_ptr<CBlockIndex>>& blocks)
{
    std::vector<PowState> result;
    result.reserve(blocks.size());
    for (const auto& block : blocks) {
        result.push_back({block->pprev, block->nHeight, block->nTime, block->nBits, block->nChainWork});
    }
    return result;
}

void AssertUnchanged(const std::vector<std::unique_ptr<CBlockIndex>>& blocks, const std::vector<PowState>& before)
{
    assert(blocks.size() == before.size());
    for (size_t i{0}; i < blocks.size(); ++i) {
        assert(blocks[i]->pprev == before[i].pprev);
        assert(blocks[i]->nHeight == before[i].nHeight);
        assert(blocks[i]->nTime == before[i].nTime);
        assert(blocks[i]->nBits == before[i].nBits);
        assert(blocks[i]->nChainWork == before[i].nChainWork);
    }
}

bool HasAncestor(const CBlockIndex& block, int height)
{
    const CBlockIndex* current{&block};
    while (current != nullptr && current->nHeight > height) {
        current = current->pprev;
    }
    return current != nullptr && current->nHeight == height;
}

void AssertProofOfWorkContracts(const uint256& hash, unsigned int nBits, const Consensus::Params& params)
{
    const auto expected_target{ReferenceTarget(nBits, params.powLimit)};
    const auto actual_target{DeriveTarget(nBits, params.powLimit)};
    assert(expected_target.has_value() == actual_target.has_value());
    if (expected_target) assert(*expected_target == *actual_target);

    const bool expected_result{expected_target && UintToArith256(hash) <= *expected_target};
    assert(CheckProofOfWorkImpl(hash, nBits, params) == expected_result);
    assert(CheckProofOfWorkImpl(hash, nBits, params) == expected_result);
}

void AssertEquivalentTimeContracts(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    if (GetBlockProof(tip) == 0) return;

    assert(GetBlockProofEquivalentTime(to, to, tip, params) == 0);
    const int64_t forward{GetBlockProofEquivalentTime(to, from, tip, params)};
    const int64_t reverse{GetBlockProofEquivalentTime(from, to, tip, params)};
    if (forward == std::numeric_limits<int64_t>::max()) {
        assert(reverse == -std::numeric_limits<int64_t>::max());
    } else if (forward == -std::numeric_limits<int64_t>::max()) {
        assert(reverse == std::numeric_limits<int64_t>::max());
    } else {
        assert(reverse == -forward);
    }
}

} // namespace

void initialize_pow()
{
    SelectParams(ChainType::MAIN);
}

FUZZ_TARGET(pow, .init = initialize_pow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const Consensus::Params& consensus_params = Params().GetConsensus();
    std::vector<std::unique_ptr<CBlockIndex>> blocks;
    const uint32_t fixed_time = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const uint32_t fixed_bits = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 10000) {
        const std::optional<CBlockHeader> block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
        if (!block_header) {
            continue;
        }
        CBlockIndex* previous_block = blocks.empty() ? nullptr : PickValue(fuzzed_data_provider, blocks).get();
        CBlockIndex& current_block{
            *blocks.emplace_back(std::make_unique<CBlockIndex>(*block_header))};
        {
            const int current_height = (previous_block != nullptr && previous_block->nHeight != std::numeric_limits<int>::max()) ? previous_block->nHeight + 1 : 0;
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.pprev = previous_block;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.nHeight = current_height;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                const uint32_t seconds = current_height * consensus_params.nPowTargetSpacing;
                if (!AdditionOverflow(fixed_time, seconds)) {
                    current_block.nTime = fixed_time + seconds;
                }
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.nBits = fixed_bits;
            }
            if (fuzzed_data_provider.ConsumeBool()) {
                current_block.nChainWork = previous_block != nullptr ? previous_block->nChainWork + GetBlockProof(*previous_block) : arith_uint256{0};
            } else {
                current_block.nChainWork = ConsumeArithUInt256(fuzzed_data_provider);
            }
        }
        const auto before{Snapshot(blocks)};
        {
            const arith_uint256 proof{GetBlockProof(current_block)};
            assert(proof == ReferenceBlockProof(current_block.nBits));
            assert(GetBlockProof(current_block) == proof);
            AssertUnchanged(blocks, before);

            const int64_t first_block_time{fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(0, std::numeric_limits<int64_t>::max())};
            const unsigned int calculated{CalculateNextWorkRequired(&current_block, first_block_time, consensus_params)};
            assert(CalculateNextWorkRequired(&current_block, first_block_time, consensus_params) == calculated);
            AssertUnchanged(blocks, before);

            const bool height_is_safe{current_block.nHeight != std::numeric_limits<int>::max()};
            const bool at_adjustment{height_is_safe && (current_block.nHeight + 1) % consensus_params.DifficultyAdjustmentInterval() == 0};
            const bool has_required_ancestor{height_is_safe && current_block.nHeight >= consensus_params.DifficultyAdjustmentInterval() - 1 && HasAncestor(current_block, current_block.nHeight - (consensus_params.DifficultyAdjustmentInterval() - 1))};
            if (!at_adjustment || has_required_ancestor) {
                const unsigned int next{GetNextWorkRequired(&current_block, &(*block_header), consensus_params)};
                assert(GetNextWorkRequired(&current_block, &(*block_header), consensus_params) == next);
                if (!at_adjustment) assert(next == current_block.nBits);
                AssertUnchanged(blocks, before);
            }
        }
        {
            const auto& to = PickValue(fuzzed_data_provider, blocks);
            const auto& from = PickValue(fuzzed_data_provider, blocks);
            const auto& tip = PickValue(fuzzed_data_provider, blocks);
            try {
                AssertEquivalentTimeContracts(*to, *from, *tip, consensus_params);
            } catch (const uint_error&) {
            }
        }
        {
            const std::optional<uint256> hash = ConsumeDeserializable<uint256>(fuzzed_data_provider);
            if (hash) {
                AssertProofOfWorkContracts(*hash, fuzzed_data_provider.ConsumeIntegral<unsigned int>(), consensus_params);
            }
        }
    }
}


FUZZ_TARGET(pow_transition, .init = initialize_pow)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const Consensus::Params& consensus_params{Params().GetConsensus()};
    std::vector<std::unique_ptr<CBlockIndex>> blocks;

    const uint32_t old_time{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
    const uint32_t new_time{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
    const int32_t version{fuzzed_data_provider.ConsumeIntegral<int32_t>()};
    uint32_t nbits{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};

    const arith_uint256 pow_limit = UintToArith256(consensus_params.powLimit);
    const auto old_target{DecodeCompactTarget(nbits)};
    if (!old_target) return;
    if (*old_target > pow_limit) {
        nbits = pow_limit.GetCompact();
    }
    // Create one difficulty adjustment period worth of headers
    for (int height = 0; height < consensus_params.DifficultyAdjustmentInterval(); ++height) {
        CBlockHeader header;
        header.nVersion = version;
        header.nTime = old_time;
        header.nBits = nbits;
        if (height == consensus_params.DifficultyAdjustmentInterval() - 1) {
            header.nTime = new_time;
        }
        auto current_block{std::make_unique<CBlockIndex>(header)};
        current_block->pprev = blocks.empty() ? nullptr : blocks.back().get();
        current_block->nHeight = height;
        blocks.emplace_back(std::move(current_block));
    }
    auto last_block{blocks.back().get()};
    unsigned int new_nbits{GetNextWorkRequired(last_block, nullptr, consensus_params)};
    assert(GetNextWorkRequired(last_block, nullptr, consensus_params) == new_nbits);
    Assert(PermittedDifficultyTransition(consensus_params, last_block->nHeight + 1, last_block->nBits, new_nbits));
}
