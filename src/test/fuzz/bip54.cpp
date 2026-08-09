// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <consensus/tx_verify.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace {

constexpr uint8_t REFERENCE_MAX_MULTISIG_PUBKEYS{20};
constexpr uint64_t REFERENCE_MAX_TX_SIGOPS{2500};

/** Unlike CCoinsViewCache::AddCoin, this backing view retains unspendable outputs, so the check
 *  scans exactly the fuzzer-provided scripts. */
class FuzzCoinsView final : public CCoinsViewBacked
{
public:
    FuzzCoinsView() : CCoinsViewBacked{&CoinsViewEmpty::Get()} {}

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override
    {
        const auto it{m_coins.find(outpoint)};
        return it == m_coins.end() ? std::nullopt : std::optional<Coin>{it->second};
    }

    void AddCoin(const COutPoint& outpoint, Coin&& coin)
    {
        m_coins.emplace(outpoint, std::move(coin));
    }

private:
    std::map<COutPoint, Coin> m_coins;
};

// The reference accounting below deliberately re-implements script parsing byte by byte rather
// than reusing CScript::GetOp or GetSigOpCount, so a production parser bug cannot cancel out of
// this differential check.
bool ReadOpcode(const CScript& script, size_t& cursor, uint8_t& opcode, std::vector<uint8_t>* pushed_data = nullptr)
{
    if (pushed_data) pushed_data->clear();
    if (cursor >= script.size()) return false;

    opcode = script[cursor++];
    if (opcode > OP_PUSHDATA4) return true;

    uint64_t pushed_size{0};
    size_t size_bytes{0};
    if (opcode < OP_PUSHDATA1) {
        pushed_size = opcode;
    } else if (opcode == OP_PUSHDATA1) {
        size_bytes = 1;
    } else if (opcode == OP_PUSHDATA2) {
        size_bytes = 2;
    } else {
        size_bytes = 4;
    }

    if (size_bytes > script.size() - cursor) return false;
    for (size_t byte{0}; byte < size_bytes; ++byte) {
        pushed_size |= uint64_t{script[cursor++]} << (8 * byte);
    }
    if (pushed_size > script.size() - cursor) return false;
    const size_t pushed_size_bytes{static_cast<size_t>(pushed_size)};

    if (pushed_data) {
        pushed_data->assign(script.begin() + cursor, script.begin() + cursor + pushed_size_bytes);
    }
    cursor += pushed_size_bytes;
    return true;
}

uint64_t ReferenceSigOpCount(const CScript& script)
{
    uint64_t sigops{0};
    uint8_t previous_opcode{OP_INVALIDOPCODE};
    size_t cursor{0};
    while (cursor < script.size()) {
        uint8_t opcode;
        if (!ReadOpcode(script, cursor, opcode)) break;
        if (opcode == OP_CHECKSIG || opcode == OP_CHECKSIGVERIFY) {
            ++sigops;
        } else if (opcode == OP_CHECKMULTISIG || opcode == OP_CHECKMULTISIGVERIFY) {
            sigops += previous_opcode >= OP_1 && previous_opcode <= OP_16
                ? previous_opcode - (OP_1 - 1)
                : REFERENCE_MAX_MULTISIG_PUBKEYS;
        }
        previous_opcode = opcode;
    }
    return sigops;
}

uint64_t ReferenceP2SHSigOpCount(const CScript& script_sig)
{
    std::vector<uint8_t> last_pushed_data;
    size_t cursor{0};
    while (cursor < script_sig.size()) {
        uint8_t opcode;
        if (!ReadOpcode(script_sig, cursor, opcode, &last_pushed_data) || opcode > OP_16) return 0;
    }
    return ReferenceSigOpCount(CScript{last_pushed_data.begin(), last_pushed_data.end()});
}

bool IsReferenceP2SH(const CScript& script)
{
    return script.size() == 23 &&
           script[0] == OP_HASH160 &&
           script[1] == 20 &&
           script[22] == OP_EQUAL;
}

} // namespace

/** Check the BIP54 sigops limit for a transaction spending a fuzzer-provided list of legacy inputs. */
FUZZ_TARGET(bip54_sigops)
{
    FuzzCoinsView coins_view;
    CCoinsViewCache coins{&coins_view, /*deterministic=*/true};
    CMutableTransaction tx;
    Txid dummy_txid;

    // Use a SpanReader in place of the usual FuzzedDataProvider in order to be able
    // to seed the corpus from the unit tests.
    if (buffer.empty()) return;
    SpanReader reader{buffer};
    uint64_t expected_sigops{0};

    // Deserialize raw scriptSig and spent scriptPubKey pairs. Keeping both raw
    // lets the fuzzer cover malformed and non-push-only P2SH scriptSigs.
    try {
        uint16_t inputs_count;
        Unserialize(reader, inputs_count);
        for (uint32_t i{0}; i < inputs_count; ++i) {
            tx.vin.emplace_back(dummy_txid, i);
            Unserialize(reader, tx.vin.back().scriptSig);

            CScript script_pub_key;
            Unserialize(reader, script_pub_key);
            CTxOut prev_txo{0, std::move(script_pub_key)};
            expected_sigops += ReferenceSigOpCount(tx.vin.back().scriptSig);
            expected_sigops += IsReferenceP2SH(prev_txo.scriptPubKey)
                ? ReferenceP2SHSigOpCount(tx.vin.back().scriptSig)
                : ReferenceSigOpCount(prev_txo.scriptPubKey);
            coins_view.AddCoin(tx.vin.back().prevout, Coin(std::move(prev_txo), 0, false));
        }
    } catch (const std::ios_base::failure&) {
        return;
    }

    const CTransaction original{tx};
    const bool expected{expected_sigops <= REFERENCE_MAX_TX_SIGOPS};
    assert(Consensus::CheckSigopsBIP54(original, coins) == expected);

    // Fields unrelated to the scripts being spent must not affect the result.
    tx.version ^= 1;
    tx.nLockTime ^= 1;
    tx.vout.emplace_back(0, CScript{} << OP_CHECKSIG);
    for (auto& txin : tx.vin) {
        txin.nSequence ^= 1;
    }
    if (!tx.vin.empty()) {
        tx.vin.front().scriptWitness.stack.emplace_back(32, uint8_t{0x42});
    }
    assert(Consensus::CheckSigopsBIP54(CTransaction(tx), coins) == expected);

    // Input order must not affect the per-input sum.
    tx = CMutableTransaction{original};
    std::reverse(tx.vin.begin(), tx.vin.end());
    assert(Consensus::CheckSigopsBIP54(CTransaction(tx), coins) == expected);

    // Direct callers must not assert or invoke undefined behavior on a view
    // that does not contain the transaction's spent outputs.
    CCoinsViewCache missing_coins{&CoinsViewEmpty::Get(), /*deterministic=*/true};
    (void)Consensus::CheckSigopsBIP54(original, missing_coins);
}
