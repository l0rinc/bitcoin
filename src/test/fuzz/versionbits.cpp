// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <common/args.h>
#include <consensus/params.h>
#include <primitives/block.h>
#include <util/chaintype.h>
#include <versionbits.h>
#include <versionbits_impl.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>
#include <test/util/check.h>

namespace {
class TestConditionChecker : public VersionBitsConditionChecker
{
private:
    mutable ThresholdConditionCache m_cache;

public:
    TestConditionChecker(const Consensus::BIP9Deployment& dep) : VersionBitsConditionChecker{dep}
    {
        CHECK(dep.period > 0);
        CHECK(dep.threshold <= dep.period);
        CHECK(0 <= dep.bit && dep.bit < 32 && dep.bit < VERSIONBITS_NUM_BITS);
        CHECK(0 <= dep.min_activation_height);
    }

    ThresholdState GetStateFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateFor(pindexPrev, m_cache); }
    int GetStateSinceHeightFor(const CBlockIndex* pindexPrev) const { return AbstractThresholdConditionChecker::GetStateSinceHeightFor(pindexPrev, m_cache); }
};

/** Track blocks mined for test */
class Blocks
{
private:
    std::vector<std::unique_ptr<CBlockIndex>> m_blocks;
    const uint32_t m_start_time;
    const uint32_t m_interval;
    const int32_t m_signal;
    const int32_t m_no_signal;

public:
    Blocks(uint32_t start_time, uint32_t interval, int32_t signal, int32_t no_signal)
        : m_start_time{start_time}, m_interval{interval}, m_signal{signal}, m_no_signal{no_signal} {}

    size_t size() const { return m_blocks.size(); }

    CBlockIndex* tip() const
    {
        return m_blocks.empty() ? nullptr : m_blocks.back().get();
    }

    CBlockIndex* mine_block(bool signal)
    {
        CBlockHeader header;
        header.nVersion = signal ? m_signal : m_no_signal;
        header.nTime = m_start_time + m_blocks.size() * m_interval;
        header.nBits = 0x1d00ffff;

        auto current_block = std::make_unique<CBlockIndex>(header);
        current_block->pprev = tip();
        current_block->nHeight = m_blocks.size();
        current_block->BuildSkip();

        return m_blocks.emplace_back(std::move(current_block)).get();
    }
};

std::unique_ptr<const CChainParams> g_params;

void initialize()
{
    // this is actually comparatively slow, so only do it once
    g_params = CreateChainParams(ArgsManager{}, ChainType::MAIN);
    CHECK(g_params != nullptr);
}

constexpr uint32_t MAX_START_TIME = 4102444800; // 2100-01-01

FUZZ_TARGET(versionbits, .init = initialize)
{
    const CChainParams& params = *g_params;
    const int64_t interval = params.GetConsensus().nPowTargetSpacing;
    CHECK(interval > 1); // need to be able to halve it
    CHECK(interval < std::numeric_limits<int32_t>::max());

    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    // making period/max_periods larger slows these tests down significantly
    const uint32_t period = 32;
    const size_t max_periods = 16;
    const size_t max_blocks = 2 * period * max_periods;

    // too many blocks at 10min each might cause uint32_t time to overflow if
    // block_start_time is at the end of the range above
    CHECK(std::numeric_limits<uint32_t>::max() - MAX_START_TIME > interval * max_blocks);

    const int64_t block_start_time = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(params.GenesisBlock().nTime, MAX_START_TIME);

    // what values for version will we use to signal / not signal?
    const int32_t ver_signal = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    const int32_t ver_nosignal = fuzzed_data_provider.ConsumeIntegral<int32_t>();
    if (ver_nosignal < 0) return; // negative values are uninteresting

    // Now that we have chosen time and versions, setup to mine blocks
    Blocks blocks(block_start_time, interval, ver_signal, ver_nosignal);

    const bool always_active_test = fuzzed_data_provider.ConsumeBool();
    const bool never_active_test = !always_active_test && fuzzed_data_provider.ConsumeBool();

    const Consensus::BIP9Deployment dep{[&]() {
        Consensus::BIP9Deployment dep;
        dep.period = period;

        dep.threshold = fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, period);
        CHECK(0 < dep.threshold && dep.threshold <= dep.period); // must be able to both pass and fail threshold!

        // select deployment parameters: bit, start time, timeout
        dep.bit = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, VERSIONBITS_NUM_BITS - 1);

        if (always_active_test) {
            dep.nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
            dep.nTimeout = fuzzed_data_provider.ConsumeBool() ? Consensus::BIP9Deployment::NO_TIMEOUT : fuzzed_data_provider.ConsumeIntegral<int64_t>();
        } else if (never_active_test) {
            dep.nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
            dep.nTimeout = fuzzed_data_provider.ConsumeBool() ? Consensus::BIP9Deployment::NO_TIMEOUT : fuzzed_data_provider.ConsumeIntegral<int64_t>();
        } else {
            // pick the timestamp to switch based on a block
            // note states will change *after* these blocks because mediantime lags
            int start_block = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * (max_periods - 3));
            int end_block = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * (max_periods - 3));

            dep.nStartTime = block_start_time + start_block * interval;
            dep.nTimeout = block_start_time + end_block * interval;

            // allow for times to not exactly match a block
            if (fuzzed_data_provider.ConsumeBool()) dep.nStartTime += interval / 2;
            if (fuzzed_data_provider.ConsumeBool()) dep.nTimeout += interval / 2;
        }
        dep.min_activation_height = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, period * max_periods);
        return dep;
    }()};
    TestConditionChecker checker(dep);

    // Early exit if the versions don't signal sensibly for the deployment
    if (!checker.Condition(ver_signal)) return;
    if (checker.Condition(ver_nosignal)) return;

    // TOP_BITS should ensure version will be positive and meet min
    // version requirement
    CHECK(ver_signal > 0);
    CHECK(ver_signal >= VERSIONBITS_LAST_OLD_BLOCK_VERSION);

    /* Strategy:
     *  * we will mine a final period worth of blocks, with
     *    randomised signalling according to a mask
     *  * but before we mine those blocks, we will mine some
     *    randomised number of prior periods; with either all
     *    or no blocks in the period signalling
     *
     * We establish the mask first, then consume "bools" until
     * we run out of fuzz data to work out how many prior periods
     * there are and which ones will signal.
     */

    // establish the mask
    const uint32_t signalling_mask = fuzzed_data_provider.ConsumeIntegral<uint32_t>();

    // mine prior periods
    while (fuzzed_data_provider.remaining_bytes() > 0) { // early exit; no need for LIMITED_WHILE
        // all blocks in these periods either do or don't signal
        bool signal = fuzzed_data_provider.ConsumeBool();
        for (uint32_t b = 0; b < period; ++b) {
            blocks.mine_block(signal);
        }

        // don't risk exceeding max_blocks or times may wrap around
        if (blocks.size() + 2 * period > max_blocks) break;
    }
    // NOTE: fuzzed_data_provider may be fully consumed at this point and should not be used further

    // now we mine the final period and check that everything looks sane

    // count the number of signalling blocks
    uint32_t blocks_sig = 0;

    // get the info for the first block of the period
    CBlockIndex* prev = blocks.tip();
    const int exp_since = checker.GetStateSinceHeightFor(prev);
    const ThresholdState exp_state = checker.GetStateFor(prev);

    // get statistics from end of previous period, then reset
    BIP9Stats last_stats;
    last_stats.period = period;
    last_stats.threshold = dep.threshold;
    last_stats.count = last_stats.elapsed = 0;
    last_stats.possible = (period >= dep.threshold);
    std::vector<bool> last_signals{};

    int prev_next_height = (prev == nullptr ? 0 : prev->nHeight + 1);
    CHECK(exp_since <= prev_next_height);

    // mine (period-1) blocks and check state
    for (uint32_t b = 1; b < period; ++b) {
        const bool signal = (signalling_mask >> (b % 32)) & 1;
        if (signal) ++blocks_sig;

        CBlockIndex* current_block = blocks.mine_block(signal);

        // verify that signalling attempt was interpreted correctly
        CHECK(checker.Condition(current_block->nVersion) == signal);

        // state and since don't change within the period
        const ThresholdState state = checker.GetStateFor(current_block);
        const int since = checker.GetStateSinceHeightFor(current_block);
        CHECK(state == exp_state);
        CHECK(since == exp_since);

        // check that after mining this block stats change as expected
        std::vector<bool> signals;
        const BIP9Stats stats = checker.GetStateStatisticsFor(current_block, &signals);
        const BIP9Stats stats_no_signals = checker.GetStateStatisticsFor(current_block);
        CHECK(stats.period == stats_no_signals.period && stats.threshold == stats_no_signals.threshold
               && stats.elapsed == stats_no_signals.elapsed && stats.count == stats_no_signals.count
               && stats.possible == stats_no_signals.possible);

        CHECK(stats.period == period);
        CHECK(stats.threshold == dep.threshold);
        CHECK(stats.elapsed == b);
        CHECK(stats.count == last_stats.count + (signal ? 1 : 0));
        CHECK(stats.possible == (stats.count + period >= stats.elapsed + dep.threshold));
        last_stats = stats;

        CHECK(signals.size() == last_signals.size() + 1);
        CHECK(signals.back() == signal);
        last_signals.push_back(signal);
        CHECK(signals == last_signals);
    }

    if (exp_state == ThresholdState::STARTED) {
        // double check that stats.possible is sane
        if (blocks_sig >= dep.threshold - 1) CHECK(last_stats.possible);
    }

    // mine the final block
    bool signal = (signalling_mask >> (period % 32)) & 1;
    if (signal) ++blocks_sig;
    CBlockIndex* current_block = blocks.mine_block(signal);
    CHECK(checker.Condition(current_block->nVersion) == signal);

    const BIP9Stats stats = checker.GetStateStatisticsFor(current_block);
    CHECK(stats.period == period);
    CHECK(stats.threshold == dep.threshold);
    CHECK(stats.elapsed == period);
    CHECK(stats.count == blocks_sig);
    CHECK(stats.possible == (stats.count + period >= stats.elapsed + dep.threshold));

    // More interesting is whether the state changed.
    const ThresholdState state = checker.GetStateFor(current_block);
    const int since = checker.GetStateSinceHeightFor(current_block);

    // since is straightforward:
    CHECK(since % period == 0);
    CHECK(0 <= since && since <= current_block->nHeight + 1);
    if (state == exp_state) {
        CHECK(since == exp_since);
    } else {
        CHECK(since == current_block->nHeight + 1);
    }

    // state is where everything interesting is
    [&]() {
        switch (state) {
        case ThresholdState::DEFINED:
            CHECK(since == 0);
            CHECK(exp_state == ThresholdState::DEFINED);
            CHECK(current_block->GetMedianTimePast() < dep.nStartTime);
            return;
        case ThresholdState::STARTED:
            CHECK(current_block->GetMedianTimePast() >= dep.nStartTime);
            if (exp_state == ThresholdState::STARTED) {
                CHECK(blocks_sig < dep.threshold);
                CHECK(current_block->GetMedianTimePast() < dep.nTimeout);
            } else {
                CHECK(exp_state == ThresholdState::DEFINED);
            }
            return;
        case ThresholdState::LOCKED_IN:
            if (exp_state == ThresholdState::LOCKED_IN) {
                CHECK(current_block->nHeight + 1 < dep.min_activation_height);
            } else {
                CHECK(exp_state == ThresholdState::STARTED);
                CHECK(blocks_sig >= dep.threshold);
            }
            return;
        case ThresholdState::ACTIVE:
            CHECK(always_active_test || dep.min_activation_height <= current_block->nHeight + 1);
            CHECK(exp_state == ThresholdState::ACTIVE || exp_state == ThresholdState::LOCKED_IN);
            return;
        case ThresholdState::FAILED:
            CHECK(never_active_test || current_block->GetMedianTimePast() >= dep.nTimeout);
            if (exp_state == ThresholdState::STARTED) {
                CHECK(blocks_sig < dep.threshold);
            } else {
                CHECK(exp_state == ThresholdState::FAILED);
            }
            return;
        } // no default case, so the compiler can warn about missing cases
        CHECK(false);
    }();

    if (blocks.size() >= period * max_periods) {
        // we chose the timeout (and block times) so that by the time we have this many blocks it's all over
        CHECK(state == ThresholdState::ACTIVE || state == ThresholdState::FAILED);
    }

    if (always_active_test) {
        // "always active" has additional restrictions
        CHECK(state == ThresholdState::ACTIVE);
        CHECK(exp_state == ThresholdState::ACTIVE);
        CHECK(since == 0);
    } else if (never_active_test) {
        // "never active" does too
        CHECK(state == ThresholdState::FAILED);
        CHECK(exp_state == ThresholdState::FAILED);
        CHECK(since == 0);
    } else {
        // for signalled deployments, the initial state is always DEFINED
        CHECK(since > 0 || state == ThresholdState::DEFINED);
        CHECK(exp_since > 0 || exp_state == ThresholdState::DEFINED);
    }
}
} // namespace
