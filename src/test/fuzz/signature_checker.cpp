// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/script.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
uint64_t Mix(uint64_t state, uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9;
    value = (value ^ (value >> 27)) * 0x94d049bb133111eb;
    return state ^ (value ^ (value >> 31));
}

template <typename Range>
uint64_t MixBytes(uint64_t state, const Range& bytes) noexcept
{
    state = Mix(state, bytes.size());
    for (const auto byte : bytes) {
        state = Mix(state, static_cast<unsigned char>(byte));
    }
    return state;
}

class DeterministicSignatureChecker : public BaseSignatureChecker
{
    const uint64_t m_seed;

    bool Result(uint64_t state, uint64_t tag, SigVersion sigversion) const noexcept
    {
        state = Mix(state, m_seed);
        state = Mix(state, tag);
        state = Mix(state, static_cast<uint64_t>(sigversion));
        return state & 1;
    }

public:
    explicit DeterministicSignatureChecker(uint64_t seed) : m_seed(seed)
    {
    }

    bool CheckECDSASignature(const std::vector<unsigned char>& scriptSig, const std::vector<unsigned char>& vchPubKey, const CScript& scriptCode, SigVersion sigversion) const override
    {
        uint64_t state{0};
        state = MixBytes(state, scriptSig);
        state = MixBytes(state, vchPubKey);
        state = MixBytes(state, scriptCode);
        return Result(state, /*tag=*/0, sigversion);
    }

    bool CheckSchnorrSignature(std::span<const unsigned char> sig, std::span<const unsigned char> pubkey, SigVersion sigversion, ScriptExecutionData& execdata, ScriptError* serror = nullptr) const override
    {
        uint64_t state{0};
        state = MixBytes(state, sig);
        state = MixBytes(state, pubkey);
        state = Mix(state, execdata.m_codeseparator_pos_init);
        if (execdata.m_codeseparator_pos_init) {
            state = Mix(state, execdata.m_codeseparator_pos);
        }
        return Result(state, /*tag=*/1, sigversion);
    }

    bool CheckLockTime(const CScriptNum& nLockTime) const override
    {
        return Result(Mix(0, static_cast<uint64_t>(nLockTime.GetInt64())), /*tag=*/2, SigVersion::BASE);
    }

    bool CheckSequence(const CScriptNum& nSequence) const override
    {
        return Result(Mix(0, static_cast<uint64_t>(nSequence.GetInt64())), /*tag=*/3, SigVersion::BASE);
    }

    ~DeterministicSignatureChecker() override = default;
};

struct ScriptResult
{
    bool success;
    ScriptError error;
    std::vector<std::vector<unsigned char>> stack;
};

void AssertEqual(const ScriptResult& a, const ScriptResult& b)
{
    assert(a.success == b.success);
    assert(a.error == b.error);
    assert(a.stack == b.stack);
}

ScriptResult RunEvalScriptWithExecData(const CScript& script, script_verify_flags flags, const BaseSignatureChecker& checker, SigVersion sigversion)
{
    ScriptResult result{/*success=*/false, SCRIPT_ERR_UNKNOWN_ERROR, {}};
    ScriptExecutionData execdata;
    result.success = EvalScript(result.stack, script, flags, checker, sigversion, execdata, &result.error);
    return result;
}

ScriptResult RunEvalScriptWrapper(const CScript& script, script_verify_flags flags, const BaseSignatureChecker& checker, SigVersion sigversion)
{
    ScriptResult result{/*success=*/false, SCRIPT_ERR_UNKNOWN_ERROR, {}};
    result.success = EvalScript(result.stack, script, flags, checker, sigversion, &result.error);
    return result;
}

std::pair<bool, ScriptError> RunVerifyScript(const CScript& script_sig, const CScript& script_pub_key, script_verify_flags flags, const BaseSignatureChecker& checker)
{
    ScriptError error{SCRIPT_ERR_UNKNOWN_ERROR};
    const bool success{VerifyScript(script_sig, script_pub_key, nullptr, flags, checker, &error)};
    return {success, error};
}
} // namespace

FUZZ_TARGET(signature_checker)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const auto flags = script_verify_flags::from_int(fuzzed_data_provider.ConsumeIntegral<script_verify_flags::value_type>());
    const SigVersion sig_version = fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0});
    const uint64_t checker_seed{fuzzed_data_provider.ConsumeIntegral<uint64_t>()};
    const auto script_1{ConsumeScript(fuzzed_data_provider)};
    const auto script_2{ConsumeScript(fuzzed_data_provider)};

    const DeterministicSignatureChecker checker{checker_seed};
    const auto direct_eval{RunEvalScriptWithExecData(script_1, flags, checker, sig_version)};
    AssertEqual(direct_eval, RunEvalScriptWrapper(script_1, flags, checker, sig_version));
    AssertEqual(direct_eval, RunEvalScriptWithExecData(script_1, flags, checker, sig_version));

    if (!IsValidFlagCombination(flags)) {
        return;
    }
    const auto verify_result{RunVerifyScript(script_1, script_2, flags, checker)};
    assert(verify_result == RunVerifyScript(script_1, script_2, flags, checker));
}
