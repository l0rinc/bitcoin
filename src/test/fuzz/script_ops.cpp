// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

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
    const unsigned int legacy_sigops{script.GetSigOpCount(false)};
    const unsigned int accurate_sigops{script.GetSigOpCount(true)};
    const unsigned int p2sh_sigops{script.GetSigOpCount(script)};
    assert(accurate_sigops <= legacy_sigops);

    const bool valid_ops{script.HasValidOps()};
    const bool is_p2sh{script.IsPayToScriptHash()};
    const bool is_p2a{script.IsPayToAnchor()};
    const bool is_p2wsh{script.IsPayToWitnessScriptHash()};
    const bool is_p2tr{script.IsPayToTaproot()};
    assert(!is_p2sh || valid_ops);
    if (!is_p2sh) {
        assert(p2sh_sigops == accurate_sigops);
    } else {
        assert(script.size() == 23);
        assert(script[0] == OP_HASH160);
        assert(script[1] == 0x14);
        assert(script[22] == OP_EQUAL);
    }

    assert(script.IsPushOnly() == script.IsPushOnly(script.begin()));
    (void)script.IsUnspendable();
    {
        CScript::const_iterator pc = script.begin();
        opcodetype opcode;
        const bool getop_without_data{script.GetOp(pc, opcode)};
        std::vector<uint8_t> data;
        CScript::const_iterator pc_data = script.begin();
        opcodetype opcode_data;
        const bool getop_with_data{script.GetOp(pc_data, opcode_data, data)};
        assert(getop_without_data == getop_with_data);
        if (getop_without_data) {
            assert(pc == pc_data);
            assert(opcode == opcode_data);
        }
        (void)script.IsPushOnly(pc);
    }
    {
        int version{42};
        std::vector<uint8_t> program{0x42};
        const bool is_witness_program{script.IsWitnessProgram(version, program)};
        if (is_witness_program) {
            assert(version >= 0 && version <= 16);
            assert(program.size() >= 2 && program.size() <= 40);
            assert(script.size() == program.size() + 2);
            assert(script[1] == program.size());
            assert(version == CScript::DecodeOP_N(static_cast<opcodetype>(script[0])));
            if (version == 0 && program.size() == 32) {
                assert(is_p2wsh);
            }
            if (version == 1 && program.size() == 32) {
                assert(is_p2tr);
            }
            if (CScript::IsPayToAnchor(version, program)) {
                assert(is_p2a);
            }
        } else {
            assert(version == -1);
            assert(program.empty());
        }
        if (is_p2a) {
            assert(is_witness_program);
            assert(CScript::IsPayToAnchor(version, program));
        }
        if (is_p2wsh) {
            assert(is_witness_program);
            assert(version == 0);
            assert(program.size() == 32);
        }
        if (is_p2tr) {
            assert(is_witness_program);
            assert(version == 1);
            assert(program.size() == 32);
        }
    }
}
