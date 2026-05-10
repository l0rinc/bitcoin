// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <hash.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/fuzz.h>

#include <cstdint>
#include <ios>
#include <utility>

FUZZ_TARGET(policy_legacy_sigops)
{
    CCoinsViewCache coins{&CoinsViewEmpty::Get(), /*deterministic=*/true};
    CMutableTransaction tx;
    Txid dummy_txid;

    if (buffer.empty()) return;
    SpanReader reader{buffer};

    try {
        uint16_t input_count;
        Unserialize(reader, input_count);
        if (input_count > 10'000) return;

        for (uint32_t i{0}; i < input_count; ++i) {
            tx.vin.emplace_back(dummy_txid, i);
            Unserialize(reader, tx.vin.back().scriptSig);

            CScript spent_script;
            Unserialize(reader, spent_script);

            bool is_p2sh;
            Unserialize(reader, is_p2sh);
            if (is_p2sh) {
                tx.vin.back().scriptSig << ToByteVector(spent_script);
            }

            CTxOut prevout{
                0,
                is_p2sh ? CScript{} << OP_HASH160 << Hash160(spent_script) << OP_EQUAL : std::move(spent_script),
            };
            coins.AddCoin(tx.vin.back().prevout, Coin{std::move(prevout), 0, false}, /*possible_overwrite=*/false);
        }
    } catch (const std::ios_base::failure&) {
        return;
    }

    (void)ValidateInputsStandardness(CTransaction{tx}, coins);
}
