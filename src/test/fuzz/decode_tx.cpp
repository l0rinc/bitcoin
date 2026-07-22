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

CMutableTransaction SentinelTransaction()
{
    CMutableTransaction tx;
    tx.version = 0x11223344;
    tx.nLockTime = 0x55667788;
    CScript script_sig;
    script_sig << OP_1;
    tx.vin.emplace_back(COutPoint{}, script_sig, 7);
    CScript script_pubkey;
    script_pubkey << OP_RETURN;
    tx.vout.emplace_back(42, script_pubkey);
    return tx;
}

bool HasSaneScripts(const CMutableTransaction& tx)
{
    if (!CTransaction(tx).IsCoinBase()) {
        for (const CTxIn& txin : tx.vin) {
            if (!txin.scriptSig.HasValidOps() || txin.scriptSig.size() > MAX_SCRIPT_SIZE) return false;
        }
    }
    for (const CTxOut& txout : tx.vout) {
        if (!txout.scriptPubKey.HasValidOps() || txout.scriptPubKey.size() > MAX_SCRIPT_SIZE) return false;
    }
    return true;
}

void AssertRoundTrip(const CMutableTransaction& tx)
{
    const CTransaction immutable{tx};
    const std::string encoded{EncodeHexTx(immutable)};
    CMutableTransaction reparsed;
    assert(DecodeHexTx(reparsed, encoded, /*try_no_witness=*/true, /*try_witness=*/true));
    assert(CTransaction(reparsed) == immutable);
    assert(EncodeHexTx(CTransaction(reparsed)) == encoded);
}

void AssertFailurePreserves(const std::string& tx_hex, const bool try_no_witness, const bool try_witness)
{
    const CMutableTransaction original{SentinelTransaction()};
    CMutableTransaction actual{original};
    assert(!DecodeHexTx(actual, tx_hex, try_no_witness, try_witness));
    assert(CTransaction(actual) == CTransaction(original));
}

} // namespace

FUZZ_TARGET(decode_tx)
{
    const std::string tx_hex = HexStr(buffer);
    AssertFailurePreserves(tx_hex, false, false);
    AssertFailurePreserves(tx_hex + "x", true, true);

    CMutableTransaction none_mtx{SentinelTransaction()};
    const CMutableTransaction none_before{none_mtx};
    const bool result_none = DecodeHexTx(none_mtx, tx_hex, false, false);
    assert(!result_none);
    assert(CTransaction(none_mtx) == CTransaction(none_before));

    CMutableTransaction witness_mtx{SentinelTransaction()};
    const CMutableTransaction witness_before{witness_mtx};
    const bool result_try_witness = DecodeHexTx(witness_mtx, tx_hex, false, true);
    if (!result_try_witness) assert(CTransaction(witness_mtx) == CTransaction(witness_before));

    CMutableTransaction both_mtx{SentinelTransaction()};
    const CMutableTransaction both_before{both_mtx};
    const bool result_try_witness_and_maybe_no_witness = DecodeHexTx(both_mtx, tx_hex, true, true);
    if (!result_try_witness_and_maybe_no_witness) assert(CTransaction(both_mtx) == CTransaction(both_before));

    CMutableTransaction no_witness_mtx{SentinelTransaction()};
    const CMutableTransaction no_witness_before{no_witness_mtx};
    const bool result_try_no_witness = DecodeHexTx(no_witness_mtx, tx_hex, true, false);
    if (!result_try_no_witness) assert(CTransaction(no_witness_mtx) == CTransaction(no_witness_before));

    if (result_try_witness) {
        AssertRoundTrip(witness_mtx);
    }
    if (result_try_no_witness) {
        assert(!no_witness_mtx.HasWitness());
        AssertRoundTrip(no_witness_mtx);
    }

    assert(result_try_witness_and_maybe_no_witness == (result_try_witness || result_try_no_witness));
    if (result_try_witness_and_maybe_no_witness) {
        assert(result_try_no_witness || result_try_witness);
        AssertRoundTrip(both_mtx);

        if (result_try_witness && result_try_no_witness) {
            const CMutableTransaction* expected{&witness_mtx};
            if (!HasSaneScripts(witness_mtx) && HasSaneScripts(no_witness_mtx)) expected = &no_witness_mtx;
            assert(CTransaction(both_mtx) == CTransaction(*expected));
        } else if (result_try_witness) {
            assert(CTransaction(both_mtx) == CTransaction(witness_mtx));
        } else {
            assert(CTransaction(both_mtx) == CTransaction(no_witness_mtx));
        }
    }
}
