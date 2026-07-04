// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <limits>

namespace {
void AssertEvalScriptErrorContract(const CScript& script, script_verify_flags flags, SigVersion sig_version)
{
    std::vector<std::vector<unsigned char>> stack_with_error;
    ScriptError error{SCRIPT_ERR_OK};
    const bool result_with_error{EvalScript(stack_with_error, script, flags, BaseSignatureChecker(), sig_version, &error)};
    assert(result_with_error == (error == SCRIPT_ERR_OK));

    std::vector<std::vector<unsigned char>> stack_without_error;
    const bool result_without_error{EvalScript(stack_without_error, script, flags, BaseSignatureChecker(), sig_version, nullptr)};
    assert(result_without_error == result_with_error);
    assert(stack_without_error == stack_with_error);
}
} // namespace

FUZZ_TARGET(eval_script)
{
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
    for (const auto sig_version : {SigVersion::BASE, SigVersion::WITNESS_V0}) {
        AssertEvalScriptErrorContract(script, flags, sig_version);
    }
}
