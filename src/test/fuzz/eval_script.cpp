// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <cassert>
#include <limits>

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
        std::vector<std::vector<unsigned char>> stack;
        ScriptError serror{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool result{EvalScript(stack, script, flags, BaseSignatureChecker(), sig_version, &serror)};
        assert(result == (serror == SCRIPT_ERR_OK));

        // BaseSignatureChecker is stateless, so executing the same script from
        // the same initial stack must produce the same observable result.
        std::vector<std::vector<unsigned char>> replay_stack;
        ScriptError replay_error{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool replay_result{EvalScript(replay_stack, script, flags, BaseSignatureChecker(), sig_version, &replay_error)};
        assert(replay_result == result);
        assert(replay_error == serror);
        assert(replay_stack == stack);
    }
}
