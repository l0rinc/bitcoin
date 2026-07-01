// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <primitives/transaction.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {
bool IsDefaultMutableTransaction(const CMutableTransaction& tx)
{
    return tx.vin.empty() &&
           tx.vout.empty() &&
           tx.version == CTransaction::CURRENT_VERSION &&
           tx.nLockTime == 0;
}

CMutableTransaction MakeNonDefaultMutableTransaction()
{
    CMutableTransaction tx;
    tx.version = 2;
    tx.nLockTime = 1;
    tx.vin.emplace_back();
    tx.vout.emplace_back(/*nValueIn=*/0, CScript{});
    return tx;
}

void AssertDecodeTxContracts(
    const std::string& tx_hex,
    bool try_no_witness,
    bool try_witness,
    bool decoded,
    const CMutableTransaction& tx)
{
    if (!decoded) {
        assert(IsDefaultMutableTransaction(tx));
        return;
    }

    assert(try_no_witness || try_witness);
    if (!try_witness) assert(!tx.HasWitness());

    CMutableTransaction trailing_tx{MakeNonDefaultMutableTransaction()};
    if (try_no_witness != try_witness) {
        assert(!DecodeHexTx(trailing_tx, tx_hex + "00", try_no_witness, try_witness));
        assert(IsDefaultMutableTransaction(trailing_tx));
    }

    const CTransaction decoded_tx{tx};
    const std::string encoded_tx{EncodeHexTx(decoded_tx)};
    CMutableTransaction roundtrip_tx;
    const bool roundtrip_try_witness{decoded_tx.HasWitness()};
    assert(DecodeHexTx(roundtrip_tx, encoded_tx,
        /*try_no_witness=*/!roundtrip_try_witness,
        /*try_witness=*/roundtrip_try_witness));
    assert(CTransaction{roundtrip_tx} == decoded_tx);
}
} // namespace

FUZZ_TARGET(decode_tx)
{
    const std::string tx_hex = HexStr(buffer);
    CMutableTransaction none_mtx{MakeNonDefaultMutableTransaction()};
    const bool result_none = DecodeHexTx(none_mtx, tx_hex, false, false);
    CMutableTransaction witness_mtx{MakeNonDefaultMutableTransaction()};
    const bool result_try_witness = DecodeHexTx(witness_mtx, tx_hex, false, true);
    CMutableTransaction witness_or_no_witness_mtx{MakeNonDefaultMutableTransaction()};
    const bool result_try_witness_and_maybe_no_witness = DecodeHexTx(witness_or_no_witness_mtx, tx_hex, true, true);
    CMutableTransaction no_witness_mtx{MakeNonDefaultMutableTransaction()};
    const bool result_try_no_witness = DecodeHexTx(no_witness_mtx, tx_hex, true, false);
    assert(!result_none);
    if (result_try_witness_and_maybe_no_witness) {
        assert(result_try_no_witness || result_try_witness);
    }
    if (result_try_no_witness) {
        assert(!no_witness_mtx.HasWitness());
        assert(result_try_witness_and_maybe_no_witness);
    }
    AssertDecodeTxContracts(
        tx_hex,
        /*try_no_witness=*/false,
        /*try_witness=*/false,
        result_none,
        none_mtx);
    AssertDecodeTxContracts(
        tx_hex,
        /*try_no_witness=*/false,
        /*try_witness=*/true,
        result_try_witness,
        witness_mtx);
    AssertDecodeTxContracts(
        tx_hex,
        /*try_no_witness=*/true,
        /*try_witness=*/true,
        result_try_witness_and_maybe_no_witness,
        witness_or_no_witness_mtx);
    AssertDecodeTxContracts(
        tx_hex,
        /*try_no_witness=*/true,
        /*try_witness=*/false,
        result_try_no_witness,
        no_witness_mtx);
}
