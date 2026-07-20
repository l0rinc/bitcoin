// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <headerssync.h>
#include <pow.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <test/util/time.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/time.h>
#include <validation.h>

#include <iterator>
#include <vector>

using State = HeadersSyncState::State;

static void initialize_headers_sync_state_fuzz()
{
    static const auto testing_setup = MakeNoLogFileContext<>(
        /*chain_type=*/ChainType::MAIN);
}

void MakeHeadersContinuous(
    const CBlockHeader& genesis_header,
    const std::vector<CBlockHeader>& all_headers,
    std::vector<CBlockHeader>& new_headers)
{
    Assume(!new_headers.empty());

    const CBlockHeader* prev_header{
        all_headers.empty() ? &genesis_header : &all_headers.back()};

    for (auto& header : new_headers) {
        header.hashPrevBlock = prev_header->GetHash();

        prev_header = &header;
    }
}

class FuzzedHeadersSyncState : public HeadersSyncState
{
public:
    FuzzedHeadersSyncState(const HeadersSyncParams& sync_params, const size_t commit_offset,
                           const CBlockIndex& chain_start, const arith_uint256& minimum_required_work)
        : HeadersSyncState(/*id=*/0, Params().GetConsensus(), sync_params, chain_start, minimum_required_work)
    {
        const_cast<size_t&>(m_commit_offset) = commit_offset;
    }
};

FUZZ_TARGET(headers_sync_state, .init = initialize_headers_sync_state_fuzz)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    CBlockHeader genesis_header{Params().GenesisBlock()};
    CBlockIndex start_index(genesis_header);

    FakeNodeClock clock{ConsumeTime(fuzzed_data_provider, /*min=*/start_index.GetMedianTimePast())};

    const uint256 genesis_hash = genesis_header.GetHash();
    start_index.phashBlock = &genesis_hash;

    const HeadersSyncParams params{
        .commitment_period = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, Params().HeadersSync().commitment_period * 2),
        .redownload_buffer_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, Params().HeadersSync().redownload_buffer_size * 2),
    };
    arith_uint256 min_work{UintToArith256(ConsumeUInt256(fuzzed_data_provider))};
    FuzzedHeadersSyncState headers_sync(
        params,
        /*commit_offset=*/fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, params.commitment_period - 1),
        /*chain_start=*/start_index,
        /*minimum_required_work=*/min_work);

    State previous_state{headers_sync.GetState()};
    uint256 redownload_released_hash{genesis_hash};

    // Store headers for potential redownload phase.
    std::vector<CBlockHeader> all_headers;
    std::vector<CBlockHeader>::const_iterator redownloaded_it;
    bool presync{true};
    bool requested_more{true};

    while (requested_more) {
        std::vector<CBlockHeader> headers;

        // Consume headers from fuzzer or maybe replay headers if we got to the
        // redownload phase.
        if (presync || fuzzed_data_provider.ConsumeBool()) {
            auto deser_headers = ConsumeDeserializable<std::vector<CBlockHeader>>(fuzzed_data_provider);
            if (!deser_headers || deser_headers->empty()) return;

            if (fuzzed_data_provider.ConsumeBool()) {
                MakeHeadersContinuous(genesis_header, all_headers, *deser_headers);
            }

            headers.swap(*deser_headers);
        } else if (auto num_headers_left{std::distance(redownloaded_it, all_headers.cend())}; num_headers_left > 0) {
            // Consume some headers from the redownload buffer (At least one
            // header is consumed).
            auto begin_it{redownloaded_it};
            std::advance(redownloaded_it, fuzzed_data_provider.ConsumeIntegralInRange<int>(1, num_headers_left));
            headers.insert(headers.cend(), begin_it, redownloaded_it);
        }

        if (headers.empty()) return;
        const auto state_before{headers_sync.GetState()};
        const auto presync_work_before{headers_sync.GetPresyncWork()};
        const auto presync_height_before{headers_sync.GetPresyncHeight()};
        const auto result = headers_sync.ProcessNextHeaders(headers, fuzzed_data_provider.ConsumeBool());
        const auto state_after{headers_sync.GetState()};
        requested_more = result.request_more;

        // ProcessNextHeaders has one live continuation state. Any failed or
        // non-continuing result must finalize the state and return no more
        // work; this is independent of the fuzzer's message construction.
        assert(result.request_more == (result.success && state_after != State::FINAL));
        assert(state_after != State::PRESYNC || state_before == State::PRESYNC);
        assert(state_after != State::REDOWNLOAD || state_before != State::FINAL);
        if (!result.success) {
            assert(state_after == State::FINAL);
            assert(result.pow_validated_headers.empty());
        }

        if (state_before == State::PRESYNC && result.success && state_after != State::FINAL) {
            arith_uint256 expected_work{presync_work_before};
            for (const auto& header : headers) expected_work += GetBlockProof(header);
            assert(headers_sync.GetPresyncWork() == expected_work);
            assert(headers_sync.GetPresyncHeight() == presync_height_before + static_cast<int64_t>(headers.size()));
            assert(headers_sync.GetPresyncTime() == headers.back().nTime);
        }

        if (!result.pow_validated_headers.empty()) {
            assert(state_before == State::REDOWNLOAD);
            for (const auto& header : result.pow_validated_headers) {
                assert(header.hashPrevBlock == redownload_released_hash);
                redownload_released_hash = header.GetHash();
            }
        }

        if (state_before == State::PRESYNC && state_after == State::REDOWNLOAD) {
            redownload_released_hash = genesis_hash;
        }

        if (result.request_more) {
            const auto locator{headers_sync.NextHeadersRequestLocator()};
            assert(!locator.vHave.empty());
            if (state_after == State::PRESYNC) {
                assert(locator.vHave.front() == headers.back().GetHash());
            } else {
                assert(state_after == State::REDOWNLOAD);
                if (state_before == State::PRESYNC) {
                    assert(locator.vHave.front() == genesis_hash);
                } else {
                    assert(locator.vHave.front() == headers.back().GetHash());
                }
            }
        }

        if (previous_state == State::REDOWNLOAD) assert(state_after != State::PRESYNC);
        previous_state = state_after;

        if (result.request_more) {
            if (presync) {
                all_headers.insert(all_headers.cend(), headers.cbegin(), headers.cend());

                if (headers_sync.GetState() == HeadersSyncState::State::REDOWNLOAD) {
                    presync = false;
                    redownloaded_it = all_headers.cbegin();

                    // If we get to redownloading, the presynced headers need
                    // to have the min amount of work on them.
                    assert(CalculateClaimedHeadersWork(all_headers) >= min_work);
                }
            }

            (void)headers_sync.NextHeadersRequestLocator();
        }
    }
}
