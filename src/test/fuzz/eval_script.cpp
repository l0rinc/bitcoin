// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <limits>
#include <utility>

namespace {
void AssertExecutionDataEqual(const ScriptExecutionData& actual, const ScriptExecutionData& expected)
{
    assert(actual.m_tapleaf_hash_init == expected.m_tapleaf_hash_init);
    if (actual.m_tapleaf_hash_init) assert(actual.m_tapleaf_hash == expected.m_tapleaf_hash);
    assert(actual.m_codeseparator_pos_init == expected.m_codeseparator_pos_init);
    if (actual.m_codeseparator_pos_init) assert(actual.m_codeseparator_pos == expected.m_codeseparator_pos);
    assert(actual.m_annex_init == expected.m_annex_init);
    if (actual.m_annex_init) {
        assert(actual.m_annex_present == expected.m_annex_present);
        assert(actual.m_annex_hash == expected.m_annex_hash);
    }
    assert(actual.m_validation_weight_left_init == expected.m_validation_weight_left_init);
    if (actual.m_validation_weight_left_init) assert(actual.m_validation_weight_left == expected.m_validation_weight_left);
    assert(actual.m_output_hash == expected.m_output_hash);
}

void AssertTapscriptAnchors()
{
    const auto Run = [](const CScript& script, ScriptError expected_error) {
        std::vector<std::vector<unsigned char>> stack;
        ScriptExecutionData execdata{};
        execdata.m_validation_weight_left = std::numeric_limits<int64_t>::max();
        execdata.m_validation_weight_left_init = true;
        ScriptError error{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool result{EvalScript(stack, script, {}, BaseSignatureChecker(), SigVersion::TAPSCRIPT, execdata, &error)};
        assert(result == (expected_error == SCRIPT_ERR_OK));
        assert(error == expected_error);
        return std::pair{std::move(stack), execdata};
    };

    const auto minimalif{Run(CScript{} << std::vector<unsigned char>{2} << OP_IF << OP_1 << OP_ENDIF, SCRIPT_ERR_TAPSCRIPT_MINIMALIF)};
    assert(minimalif.second.m_codeseparator_pos_init);

    const auto empty_pubkey{Run(CScript{} << std::vector<unsigned char>{} << std::vector<unsigned char>{} << OP_CHECKSIG, SCRIPT_ERR_TAPSCRIPT_EMPTY_PUBKEY)};
    assert(empty_pubkey.second.m_codeseparator_pos_init);

    const auto [checksigadd_stack, checksigadd_execdata]{Run(CScript{} << std::vector<unsigned char>{} << CScriptNum{7} << std::vector<unsigned char>(32, 1) << OP_CHECKSIGADD, SCRIPT_ERR_OK)};
    assert(checksigadd_stack.size() == 1);
    const CScriptNum checksigadd_result{checksigadd_stack.front(), /*fRequireMinimal=*/true};
    assert(checksigadd_result.getint() == 7);
    assert(checksigadd_execdata.m_validation_weight_left == std::numeric_limits<int64_t>::max());
}
} // namespace

FUZZ_TARGET(eval_script)
{
    AssertTapscriptAnchors();
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto flags = script_verify_flags::from_int(fuzzed_data_provider.ConsumeIntegral<script_verify_flags::value_type>());
    const std::vector<uint8_t> script_bytes = [&] {
        if (fuzzed_data_provider.remaining_bytes() != 0) {
            return fuzzed_data_provider.ConsumeRemainingBytes<uint8_t>();
        } else {
            // Avoid UBSan warning:
            //   test/fuzz/FuzzedDataProvider.h:212:17: runtime error: null pointer passed as argument 1, which is declared to never be null
            //   /usr/include/string.h:43:28: note: nonnull attribute specified here
            return std::vector<uint8_t>();
        }
    }();
    const CScript script(script_bytes.begin(), script_bytes.end());
    for (const auto sig_version : {SigVersion::BASE, SigVersion::WITNESS_V0, SigVersion::TAPSCRIPT}) {
        std::vector<std::vector<unsigned char>> stack;
        ScriptExecutionData execdata{};
        if (sig_version == SigVersion::TAPSCRIPT) {
            execdata.m_validation_weight_left = std::numeric_limits<int64_t>::max();
            execdata.m_validation_weight_left_init = true;
        }
        ScriptError serror{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool result{EvalScript(stack, script, flags, BaseSignatureChecker(), sig_version, execdata, &serror)};
        assert(result == (serror == SCRIPT_ERR_OK));

        // BaseSignatureChecker is stateless, so executing the same script from
        // the same initial stack must produce the same observable result.
        std::vector<std::vector<unsigned char>> replay_stack;
        ScriptExecutionData replay_execdata{};
        if (sig_version == SigVersion::TAPSCRIPT) {
            replay_execdata.m_validation_weight_left = std::numeric_limits<int64_t>::max();
            replay_execdata.m_validation_weight_left_init = true;
        }
        ScriptError replay_error{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool replay_result{EvalScript(replay_stack, script, flags, BaseSignatureChecker(), sig_version, replay_execdata, &replay_error)};
        assert(replay_result == result);
        assert(replay_error == serror);
        assert(replay_stack == stack);
        AssertExecutionDataEqual(replay_execdata, execdata);
    }
}
