// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <script/script.h>
#include <signet.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <vector>

namespace {

bool IsSignetToSpendScriptSig(const CScript& script)
{
    opcodetype opcode;
    CScript::const_iterator pc{script.begin()};
    std::vector<uint8_t> pushdata;
    if (!script.GetOp(pc, opcode, pushdata) || opcode != OP_0 || !pushdata.empty()) return false;
    if (!script.GetOp(pc, opcode, pushdata) || pushdata.size() != 72) return false;
    return pc == script.end();
}

void AssertSignetTxContracts(const SignetTxs& txs, const CScript& challenge)
{
    assert(txs.m_to_spend.version == 0);
    assert(txs.m_to_spend.nLockTime == 0);
    assert(txs.m_to_spend.vin.size() == 1);
    assert(txs.m_to_spend.vin[0].prevout.IsNull());
    assert(IsSignetToSpendScriptSig(txs.m_to_spend.vin[0].scriptSig));
    assert(txs.m_to_spend.vin[0].nSequence == 0);
    assert(txs.m_to_spend.vout.size() == 1);
    assert(txs.m_to_spend.vout[0].nValue == 0);
    assert(txs.m_to_spend.vout[0].scriptPubKey == challenge);

    assert(txs.m_to_sign.version == 0);
    assert(txs.m_to_sign.nLockTime == 0);
    assert(txs.m_to_sign.vin.size() == 1);
    const COutPoint expected_prevout{txs.m_to_spend.GetHash(), 0};
    assert(txs.m_to_sign.vin[0].prevout == expected_prevout);
    assert(txs.m_to_sign.vin[0].nSequence == 0);
    assert(txs.m_to_sign.vout.size() == 1);
    assert(txs.m_to_sign.vout[0].nValue == 0);
    assert(txs.m_to_sign.vout[0].scriptPubKey == CScript(OP_RETURN));
}

} // namespace

void initialize_signet()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::SIGNET);
}

FUZZ_TARGET(signet, .init = initialize_signet)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::optional<CBlock> block = ConsumeDeserializable<CBlock>(fuzzed_data_provider, TX_WITH_WITNESS);
    if (!block) {
        return;
    }
    const auto& consensus{Params().GetConsensus()};
    const CScript consensus_challenge{consensus.signet_challenge.begin(), consensus.signet_challenge.end()};
    const std::optional<SignetTxs> consensus_txs{SignetTxs::Create(*block, consensus_challenge)};
    const bool valid_solution{CheckSignetBlockSolution(*block, consensus)};
    if (block->GetHash() == consensus.hashGenesisBlock) {
        assert(valid_solution);
    } else if (!consensus_txs) {
        assert(!valid_solution);
    } else {
        AssertSignetTxContracts(*consensus_txs, consensus_challenge);
    }

    const CScript challenge{ConsumeScript(fuzzed_data_provider)};
    const std::optional<SignetTxs> signet_txs{SignetTxs::Create(*block, challenge)};
    if (signet_txs) {
        AssertSignetTxContracts(*signet_txs, challenge);
    }
}
