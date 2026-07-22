// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
class FuzzedSignatureChecker : public BaseSignatureChecker
{
    // Keep callback outcomes stable so the same script and checker state can
    // be replayed without consuming a different byte from the fuzzer input.
    const std::array<bool, 4> m_results;

public:
    explicit FuzzedSignatureChecker(std::array<bool, 4> results) : m_results(results)
    {
    }

    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
    {
        return m_results[0];
    }

    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const override
    {
        if (m_results[1]) return true;
        if (serror) *serror = SCRIPT_ERR_SCHNORR_SIG;
        return false;
    }

    bool CheckLockTime(const CScriptNum& nLockTime) const override
    {
        return m_results[2];
    }

    bool CheckSequence(const CScriptNum& nSequence) const override
    {
        return m_results[3];
    }

    virtual ~FuzzedSignatureChecker() = default;
};
} // namespace

FUZZ_TARGET(signature_checker)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto flags = script_verify_flags::from_int(fuzzed_data_provider.ConsumeIntegral<script_verify_flags::value_type>());
    const auto script_1{ConsumeScript(fuzzed_data_provider)};
    const auto script_2{ConsumeScript(fuzzed_data_provider)};

    const std::array checker_results{
        fuzzed_data_provider.ConsumeBool(), // ECDSA
        fuzzed_data_provider.ConsumeBool(), // Schnorr
        fuzzed_data_provider.ConsumeBool(), // CHECKLOCKTIMEVERIFY
        fuzzed_data_provider.ConsumeBool(), // CHECKSEQUENCEVERIFY
    };

    for (const auto sig_version : {SigVersion::BASE, SigVersion::WITNESS_V0, SigVersion::TAPSCRIPT}) {
        std::vector<std::vector<unsigned char>> stack;
        ScriptExecutionData execdata;
        if (sig_version == SigVersion::TAPSCRIPT) {
            execdata.m_validation_weight_left = std::numeric_limits<int64_t>::max();
            execdata.m_validation_weight_left_init = true;
        }
        ScriptError serror{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool result{EvalScript(stack, script_1, flags, FuzzedSignatureChecker{checker_results}, sig_version, execdata, &serror)};
        assert(result == (serror == SCRIPT_ERR_OK));

        std::vector<std::vector<unsigned char>> replay_stack;
        ScriptExecutionData replay_execdata;
        if (sig_version == SigVersion::TAPSCRIPT) {
            replay_execdata.m_validation_weight_left = std::numeric_limits<int64_t>::max();
            replay_execdata.m_validation_weight_left_init = true;
        }
        ScriptError replay_error{SCRIPT_ERR_UNKNOWN_ERROR};
        const bool replay_result{EvalScript(replay_stack, script_1, flags, FuzzedSignatureChecker{checker_results}, sig_version, replay_execdata, &replay_error)};
        assert(replay_result == result);
        assert(replay_error == serror);
        assert(replay_stack == stack);
        assert(replay_execdata.m_codeseparator_pos_init == execdata.m_codeseparator_pos_init);
        assert(!execdata.m_codeseparator_pos_init || replay_execdata.m_codeseparator_pos == execdata.m_codeseparator_pos);
        assert(replay_execdata.m_validation_weight_left_init == execdata.m_validation_weight_left_init);
        assert(!execdata.m_validation_weight_left_init || replay_execdata.m_validation_weight_left == execdata.m_validation_weight_left);
    }

    if (!IsValidFlagCombination(flags)) {
        return;
    }

    ScriptError verify_error{SCRIPT_ERR_UNKNOWN_ERROR};
    const bool verify_result{VerifyScript(script_1, script_2, nullptr, flags, FuzzedSignatureChecker{checker_results}, &verify_error)};
    assert(verify_result == (verify_error == SCRIPT_ERR_OK));

    CScriptWitness empty_witness;
    ScriptError explicit_empty_error{SCRIPT_ERR_UNKNOWN_ERROR};
    const bool explicit_empty_result{VerifyScript(script_1, script_2, &empty_witness, flags, FuzzedSignatureChecker{checker_results}, &explicit_empty_error)};
    assert(explicit_empty_result == verify_result);
    assert(explicit_empty_error == verify_error);

    ScriptError replay_error{SCRIPT_ERR_UNKNOWN_ERROR};
    const bool replay_result{VerifyScript(script_1, script_2, nullptr, flags, FuzzedSignatureChecker{checker_results}, &replay_error)};
    assert(replay_result == verify_result);
    assert(replay_error == verify_error);
}
