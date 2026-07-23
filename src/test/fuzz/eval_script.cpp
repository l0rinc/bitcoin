// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pubkey.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <limits>
#include <optional>

namespace {
void AssertEvalScriptErrorContract(const CScript& script,
                                   script_verify_flags flags,
                                   SigVersion sig_version,
                                   std::optional<ScriptError> expected_error = std::nullopt)
{
    std::vector<std::vector<unsigned char>> stack_with_error;
    ScriptError error{SCRIPT_ERR_OK};
    const bool result_with_error{EvalScript(stack_with_error, script, flags, BaseSignatureChecker(), sig_version, &error)};
    assert(result_with_error == (error == SCRIPT_ERR_OK));
    if (expected_error) assert(error == *expected_error);

    std::vector<std::vector<unsigned char>> stack_without_error;
    const bool result_without_error{EvalScript(stack_without_error, script, flags, BaseSignatureChecker(), sig_version, nullptr)};
    assert(result_without_error == result_with_error);
    assert(stack_without_error == stack_with_error);
}

void AssertFailurePathErrorContracts(SigVersion sig_version)
{
    const std::vector<unsigned char> compressed_pubkey{
        0x02, 0x86, 0x5c, 0x40, 0x29, 0x3a, 0x68, 0x0c, 0xb9, 0x02, 0x0e,
        0x7b, 0x1e, 0x10, 0x6d, 0x8c, 0x19, 0x16, 0xd3, 0xce, 0xf9, 0x9a,
        0xaa, 0x43, 0x1a, 0x56, 0xd2, 0x53, 0xe6, 0x92, 0x56, 0xda, 0xc0};

    const auto cltv_flags{SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY};
    AssertEvalScriptErrorContract(CScript{} << OP_CHECKLOCKTIMEVERIFY, cltv_flags, sig_version, SCRIPT_ERR_INVALID_STACK_OPERATION);
    AssertEvalScriptErrorContract(CScript{} << OP_1NEGATE << OP_CHECKLOCKTIMEVERIFY, cltv_flags, sig_version, SCRIPT_ERR_NEGATIVE_LOCKTIME);
    AssertEvalScriptErrorContract(CScript{} << std::vector<unsigned char>{0x80} << OP_CHECKLOCKTIMEVERIFY,
                                  cltv_flags, sig_version, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
    AssertEvalScriptErrorContract(CScript{} << OP_0 << OP_CHECKLOCKTIMEVERIFY, cltv_flags, sig_version, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
    const std::vector<unsigned char> cltv_nonminimal_bytes{0x01, 0x00, static_cast<unsigned char>(OP_CHECKLOCKTIMEVERIFY)};
    const CScript cltv_nonminimal{cltv_nonminimal_bytes.begin(), cltv_nonminimal_bytes.end()};
    AssertEvalScriptErrorContract(cltv_nonminimal, cltv_flags | SCRIPT_VERIFY_MINIMALDATA, sig_version, SCRIPT_ERR_SCRIPTNUM);
    AssertEvalScriptErrorContract(CScript{} << std::vector<unsigned char>{0x00, 0x00, 0x00, 0x00, 0x01} << OP_CHECKLOCKTIMEVERIFY,
                                  cltv_flags, sig_version, SCRIPT_ERR_UNSATISFIED_LOCKTIME);

    const auto csv_flags{SCRIPT_VERIFY_CHECKSEQUENCEVERIFY};
    AssertEvalScriptErrorContract(CScript{} << OP_CHECKSEQUENCEVERIFY, csv_flags, sig_version, SCRIPT_ERR_INVALID_STACK_OPERATION);
    AssertEvalScriptErrorContract(CScript{} << OP_1NEGATE << OP_CHECKSEQUENCEVERIFY, csv_flags, sig_version, SCRIPT_ERR_NEGATIVE_LOCKTIME);
    AssertEvalScriptErrorContract(CScript{} << std::vector<unsigned char>{0x80} << OP_CHECKSEQUENCEVERIFY,
                                  csv_flags, sig_version, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
    AssertEvalScriptErrorContract(CScript{} << OP_0 << OP_CHECKSEQUENCEVERIFY, csv_flags, sig_version, SCRIPT_ERR_UNSATISFIED_LOCKTIME);
    AssertEvalScriptErrorContract(CScript{} << std::vector<unsigned char>{0x00, 0x00, 0x00, 0x00, 0x01} << OP_CHECKSEQUENCEVERIFY,
                                  csv_flags, sig_version, SCRIPT_ERR_UNSATISFIED_LOCKTIME);

    const auto strictenc_flags{SCRIPT_VERIFY_STRICTENC};
    AssertEvalScriptErrorContract(CScript{} << OP_0 << compressed_pubkey << OP_CHECKSIGVERIFY,
                                  strictenc_flags, sig_version, SCRIPT_ERR_CHECKSIGVERIFY);
    AssertEvalScriptErrorContract(CScript{} << OP_0 << OP_0 << OP_1 << compressed_pubkey << OP_1 << OP_CHECKMULTISIGVERIFY,
                                  strictenc_flags, sig_version, SCRIPT_ERR_CHECKMULTISIGVERIFY);
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
        AssertFailurePathErrorContracts(sig_version);
    }
}
