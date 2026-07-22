// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/script.h>

#include <chainparams.h>
#include <compressor.h>
#include <core_io.h>
#include <core_memusage.h>
#include <key_io.h>
#include <policy/policy.h>
#include <pubkey.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/interpreter.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/solver.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <univalue.h>
#include <util/chaintype.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

void AssertSolverContracts(const TxoutType type, const std::vector<std::vector<unsigned char>>& solutions)
{
    switch (type) {
    case TxoutType::NONSTANDARD:
    case TxoutType::NULL_DATA:
    case TxoutType::ANCHOR:
        assert(solutions.empty());
        break;
    case TxoutType::PUBKEY:
        assert(solutions.size() == 1);
        assert(CPubKey::ValidSize(solutions[0]));
        break;
    case TxoutType::PUBKEYHASH:
    case TxoutType::SCRIPTHASH:
    case TxoutType::WITNESS_V0_KEYHASH:
        assert(solutions.size() == 1);
        assert(solutions[0].size() == 20);
        break;
    case TxoutType::WITNESS_V0_SCRIPTHASH:
    case TxoutType::WITNESS_V1_TAPROOT:
        assert(solutions.size() == 1);
        assert(solutions[0].size() == 32);
        break;
    case TxoutType::WITNESS_UNKNOWN:
        assert(solutions.size() == 2);
        assert(solutions[0].size() == 1);
        assert(solutions[0][0] >= 1 && solutions[0][0] <= 16);
        assert(solutions[1].size() >= 2 && solutions[1].size() <= 40);
        break;
    case TxoutType::MULTISIG:
        assert(solutions.size() >= 3);
        assert(solutions.front().size() == 1);
        assert(solutions.back().size() == 1);
        assert(solutions.front()[0] >= 1);
        assert(solutions.front()[0] <= solutions.back()[0]);
        assert(solutions.back()[0] <= MAX_PUBKEYS_PER_MULTISIG);
        assert(solutions.size() == static_cast<size_t>(solutions.back()[0]) + 2);
        for (size_t i{1}; i + 1 < solutions.size(); ++i) {
            assert(CPubKey::ValidSize(solutions[i]));
        }
        break;
    }
}

bool HasExtractableDestination(const TxoutType type)
{
    switch (type) {
    case TxoutType::PUBKEYHASH:
    case TxoutType::SCRIPTHASH:
    case TxoutType::WITNESS_V0_KEYHASH:
    case TxoutType::WITNESS_V0_SCRIPTHASH:
    case TxoutType::WITNESS_V1_TAPROOT:
    case TxoutType::ANCHOR:
    case TxoutType::WITNESS_UNKNOWN:
        return true;
    case TxoutType::NONSTANDARD:
    case TxoutType::PUBKEY:
    case TxoutType::MULTISIG:
    case TxoutType::NULL_DATA:
        return false;
    }
    assert(false);
    return false;
}

// Keep the witness count model separate from CountWitnessSigOps so branch regressions are visible.
size_t WitnessSigOpsModel(const CScript& script_sig, const CScript& script_pub_key, const CScriptWitness& witness, const script_verify_flags flags)
{
    if ((flags & SCRIPT_VERIFY_WITNESS) == 0) return 0;

    const auto count_witness_program = [&witness](const int version, const std::vector<unsigned char>& program) {
        if (version == 0 && program.size() == WITNESS_V0_KEYHASH_SIZE) return size_t{1};
        if (version == 0 && program.size() == WITNESS_V0_SCRIPTHASH_SIZE && !witness.stack.empty()) {
            const CScript witness_script{witness.stack.back().begin(), witness.stack.back().end()};
            return static_cast<size_t>(witness_script.GetSigOpCount(true));
        }
        return size_t{0};
    };

    int witness_version;
    std::vector<unsigned char> witness_program;
    if (script_pub_key.IsWitnessProgram(witness_version, witness_program)) {
        return count_witness_program(witness_version, witness_program);
    }

    if (!script_pub_key.IsPayToScriptHash() || !script_sig.IsPushOnly()) return 0;

    CScript::const_iterator pc{script_sig.begin()};
    std::vector<unsigned char> data;
    while (pc < script_sig.end()) {
        opcodetype opcode;
        if (!script_sig.GetOp(pc, opcode, data)) return 0;
    }
    const CScript redeem_script{data.begin(), data.end()};
    if (redeem_script.IsWitnessProgram(witness_version, witness_program)) {
        return count_witness_program(witness_version, witness_program);
    }
    return 0;
}

} // namespace

void initialize_script()
{
    SelectParams(ChainType::REGTEST);
}

FUZZ_TARGET(script, .init = initialize_script)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CScript script{ConsumeScript(fuzzed_data_provider)};

    CompressedScript compressed;
    if (CompressScript(script, compressed)) {
        const unsigned int size = compressed[0];
        compressed.erase(compressed.begin());
        assert(size <= 5);
        CScript decompressed_script;
        const bool ok = DecompressScript(decompressed_script, size, compressed);
        assert(ok);
        assert(script == decompressed_script);
    }

    TxoutType which_type;
    bool is_standard_ret = IsStandard(script, which_type);
    if (!is_standard_ret) {
        assert(which_type == TxoutType::NONSTANDARD ||
               which_type == TxoutType::NULL_DATA ||
               which_type == TxoutType::MULTISIG);
    }
    if (which_type == TxoutType::NONSTANDARD) {
        assert(!is_standard_ret);
    }
    if (which_type == TxoutType::NULL_DATA) {
        assert(script.IsUnspendable());
    }
    if (script.IsUnspendable()) {
        assert(which_type == TxoutType::NULL_DATA ||
               which_type == TxoutType::NONSTANDARD);
    }

    std::vector<std::vector<unsigned char>> solutions;
    const TxoutType solved_type{Solver(script, solutions)};
    assert(solved_type == which_type);
    AssertSolverContracts(solved_type, solutions);

    const bool expected_standard{solved_type != TxoutType::NONSTANDARD &&
                                 (solved_type != TxoutType::MULTISIG ||
                                  (solutions.back()[0] <= 3 && solutions.front()[0] >= 1 && solutions.front()[0] <= solutions.back()[0]))};
    assert(is_standard_ret == expected_standard);

    CTxDestination address;
    bool extract_destination_ret = ExtractDestination(script, address);
    const bool has_destination{HasExtractableDestination(solved_type)};
    assert(extract_destination_ret == has_destination);
    assert(IsValidDestination(address) == has_destination);
    assert(GetScriptForDestination(address) == script);
    if (!extract_destination_ret) {
        assert(which_type == TxoutType::PUBKEY ||
               which_type == TxoutType::NONSTANDARD ||
               which_type == TxoutType::NULL_DATA ||
               which_type == TxoutType::MULTISIG);
    }
    if (which_type == TxoutType::NONSTANDARD ||
        which_type == TxoutType::NULL_DATA ||
        which_type == TxoutType::MULTISIG) {
        assert(!extract_destination_ret);
    }

    const FlatSigningProvider signing_provider;
    (void)InferDescriptor(script, signing_provider);
    int witness_version;
    std::vector<unsigned char> witness_program;
    const bool is_native_witness{script.IsWitnessProgram(witness_version, witness_program)};
    assert(IsSegWitOutput(signing_provider, script) == is_native_witness);

    (void)RecursiveDynamicUsage(script);

    {
        const std::vector<uint8_t> bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        CompressedScript compressed_script;
        compressed_script.assign(bytes.begin(), bytes.end());
        // DecompressScript(..., ..., bytes) is not guaranteed to be defined if the bytes vector is too short
        if (compressed_script.size() >= 32) {
            CScript decompressed_script;
            DecompressScript(decompressed_script, fuzzed_data_provider.ConsumeIntegral<unsigned int>(), compressed_script);
        }
    }

    const std::optional<CScript> other_script = ConsumeDeserializable<CScript>(fuzzed_data_provider);
    if (other_script) {
        {
            CScript script_mut{script};
            (void)FindAndDelete(script_mut, *other_script);
        }
        const std::vector<std::string> random_string_vector = ConsumeRandomLengthStringVector(fuzzed_data_provider);
        const auto flags_rand{fuzzed_data_provider.ConsumeIntegral<script_verify_flags::value_type>()};
        const auto flags = script_verify_flags::from_int(flags_rand) | SCRIPT_VERIFY_P2SH;
        {
            CScriptWitness wit;
            for (const auto& s : random_string_vector) {
                wit.stack.emplace_back(s.begin(), s.end());
            }
            const size_t expected_sigops{WitnessSigOpsModel(script, *other_script, wit, flags)};
            assert(CountWitnessSigOps(script, *other_script, wit, flags) == expected_sigops);
            wit.SetNull();
        }
    }

    (void)GetOpName(ConsumeOpcodeType(fuzzed_data_provider));
    (void)ScriptErrorString(static_cast<ScriptError>(fuzzed_data_provider.ConsumeIntegralInRange<int>(0, SCRIPT_ERR_ERROR_COUNT)));

    {
        const std::vector<uint8_t> bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        CScript append_script{bytes.begin(), bytes.end()};
        append_script << fuzzed_data_provider.ConsumeIntegral<int64_t>();
        append_script << ConsumeOpcodeType(fuzzed_data_provider);
        append_script << CScriptNum{fuzzed_data_provider.ConsumeIntegral<int64_t>()};
        append_script << ConsumeRandomLengthByteVector(fuzzed_data_provider);
    }

    {
        const CTxDestination tx_destination_1{
            fuzzed_data_provider.ConsumeBool() ?
                DecodeDestination(fuzzed_data_provider.ConsumeRandomLengthString()) :
                ConsumeTxDestination(fuzzed_data_provider)};
        const CTxDestination tx_destination_2{ConsumeTxDestination(fuzzed_data_provider)};
        const std::string encoded_dest{EncodeDestination(tx_destination_1)};
        const UniValue json_dest{DescribeAddress(tx_destination_1)};
        (void)GetKeyForDestination(/*store=*/{}, tx_destination_1);
        const CScript dest{GetScriptForDestination(tx_destination_1)};
        const bool valid{IsValidDestination(tx_destination_1)};

        if (!std::get_if<PubKeyDestination>(&tx_destination_1)) {
            // Only try to round trip non-pubkey destinations since PubKeyDestination has no encoding
            Assert(dest.empty() != valid);
            Assert(tx_destination_1 == DecodeDestination(encoded_dest));
            Assert(valid == IsValidDestinationString(encoded_dest));
        }

        (void)(tx_destination_1 < tx_destination_2);
        if (tx_destination_1 == tx_destination_2) {
            Assert(encoded_dest == EncodeDestination(tx_destination_2));
            Assert(json_dest.write() == DescribeAddress(tx_destination_2).write());
            Assert(dest == GetScriptForDestination(tx_destination_2));
        }
    }
}
