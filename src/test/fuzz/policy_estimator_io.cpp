// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees/block_policy_estimator.h>
#include <policy/fees/block_policy_estimator_args.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstddef>
#include <cstdio>
#include <memory>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;

std::vector<std::byte> CaptureEstimatorState(CBlockPolicyEstimator& estimator)
{
    AutoFile file{std::tmpfile()};
    Assert(!file.IsNull());
    Assert(estimator.Write(file));
    const auto size{file.size()};
    file.seek(0, SEEK_SET);
    std::vector<std::byte> state(size);
    file.read(state);
    Assert(file.fclose() == 0);
    return state;
}
} // namespace

void initialize_policy_estimator_io()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(policy_estimator_io, .init = initialize_policy_estimator_io)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};
    AutoFile fuzzed_auto_file{fuzzed_file_provider.open()};
    // Reusing block_policy_estimator across runs to avoid costly creation of CBlockPolicyEstimator object.
    static CBlockPolicyEstimator block_policy_estimator{FeeestPath(*g_setup->m_node.args), DEFAULT_ACCEPT_STALE_FEE_ESTIMATES};
    const auto before_read{CaptureEstimatorState(block_policy_estimator)};
    const bool read_succeeded{block_policy_estimator.Read(fuzzed_auto_file)};
    if (!read_succeeded) {
        assert(CaptureEstimatorState(block_policy_estimator) == before_read);
    } else {
        const auto before_write{CaptureEstimatorState(block_policy_estimator)};
        block_policy_estimator.Write(fuzzed_auto_file);
        assert(CaptureEstimatorState(block_policy_estimator) == before_write);
    }
    (void)fuzzed_auto_file.fclose();
}
