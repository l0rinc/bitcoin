// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/check.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

void AssertScriptContracts(const CScript& script)
{
    bool has_valid_ops{true};
    bool is_push_only{true};
    CScript::const_iterator pc{script.begin()};
    while (pc < script.end()) {
        const CScript::const_iterator previous_pc{pc};
        opcodetype opcode;
        std::vector<unsigned char> data;
        if (!script.GetOp(pc, opcode, data)) {
            has_valid_ops = false;
            is_push_only = false;
            break;
        }
        Assert(pc > previous_pc);
        if (opcode > MAX_OPCODE || data.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            has_valid_ops = false;
        }
        if (opcode > OP_16) {
            is_push_only = false;
        }
    }
    Assert(has_valid_ops == script.HasValidOps());
    Assert(is_push_only == script.IsPushOnly());

    CScript::const_iterator pc_with_data{script.begin()};
    CScript::const_iterator pc_without_data{script.begin()};
    while (pc_with_data < script.end() || pc_without_data < script.end()) {
        const CScript::const_iterator previous_pc{pc_with_data};
        opcodetype opcode_with_data;
        opcodetype opcode_without_data;
        std::vector<unsigned char> data;
        const bool with_data{script.GetOp(pc_with_data, opcode_with_data, data)};
        const bool without_data{script.GetOp(pc_without_data, opcode_without_data)};
        Assert(with_data == without_data);
        Assert(opcode_with_data == opcode_without_data);
        Assert(pc_with_data == pc_without_data);
        if (!with_data) break;
        Assert(pc_with_data > previous_pc);
    }

    const unsigned int sigops_accurate{script.GetSigOpCount(true)};
    const unsigned int sigops_inaccurate{script.GetSigOpCount(false)};
    Assert(sigops_accurate <= sigops_inaccurate);

    int witness_version{-1};
    std::vector<unsigned char> witness_program;
    const bool is_witness_program{script.IsWitnessProgram(witness_version, witness_program)};
    if (is_witness_program) {
        Assert(witness_version >= 0 && witness_version <= 16);
        Assert(witness_program.size() >= 2 && witness_program.size() <= 40);
        CScript reconstructed;
        reconstructed << CScript::EncodeOP_N(witness_version) << witness_program;
        Assert(reconstructed == script);
        Assert(script.IsPayToAnchor() == CScript::IsPayToAnchor(witness_version, witness_program));
        Assert(script.IsPayToWitnessScriptHash() == (witness_version == 0 && witness_program.size() == 32));
        Assert(script.IsPayToTaproot() == (witness_version == 1 && witness_program.size() == 32));
    } else {
        Assert(!script.IsPayToAnchor());
        Assert(!script.IsPayToWitnessScriptHash());
        Assert(!script.IsPayToTaproot());
    }
}

} // namespace

FUZZ_TARGET(script_ops)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    CScript script_mut = ConsumeScript(fuzzed_data_provider);
    LIMITED_WHILE (fuzzed_data_provider.remaining_bytes() > 0, 1000000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                CScript s = ConsumeScript(fuzzed_data_provider);
                script_mut = std::move(s);
            },
            [&] {
                const CScript& s = ConsumeScript(fuzzed_data_provider);
                script_mut = s;
            },
            [&] {
                script_mut << fuzzed_data_provider.ConsumeIntegral<int64_t>();
            },
            [&] {
                script_mut << ConsumeOpcodeType(fuzzed_data_provider);
            },
            [&] {
                script_mut << ConsumeScriptNum(fuzzed_data_provider);
            },
            [&] {
                script_mut << ConsumeRandomLengthByteVector(fuzzed_data_provider);
            },
            [&] {
                script_mut.clear();
            });
    }
    const CScript& script = script_mut;
    AssertScriptContracts(script);
    (void)script.GetSigOpCount(false);
    (void)script.GetSigOpCount(true);
    (void)script.GetSigOpCount(script);
    (void)script.HasValidOps();
    (void)script.IsPayToScriptHash();
    (void)script.IsPayToAnchor();
    (void)script.IsPayToWitnessScriptHash();
    (void)script.IsPushOnly();
    (void)script.IsUnspendable();
    {
        CScript::const_iterator pc = script.begin();
        opcodetype opcode;
        (void)script.GetOp(pc, opcode);
        std::vector<uint8_t> data;
        (void)script.GetOp(pc, opcode, data);
        (void)script.IsPushOnly(pc);
    }
    {
        int version;
        std::vector<uint8_t> program;
        (void)script.IsWitnessProgram(version, program);
    }
}
