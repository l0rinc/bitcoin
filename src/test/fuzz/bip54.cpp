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

#include <cstdint>
#include <map>
#include <optional>
#include <utility>

namespace {

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
            coins_view.AddCoin(tx.vin.back().prevout, Coin(std::move(prev_txo), 0, false));
        }
    } catch (const std::ios_base::failure&) {
        return;
    }

    (void)Consensus::CheckSigopsBIP54(CTransaction(tx), coins);
}
