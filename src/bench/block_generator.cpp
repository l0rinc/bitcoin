// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license.

#include "block_generator.h"

#include <addresstype.h>
#include <chainparams.h>
#include <consensus/merkle.h>
#include <key.h>
#include <pow.h>
#include <primitives/block.h>
#include <random.h>
#include <script/script.h>
#include <script/solver.h>
#include <streams.h>
#include <test/util/transaction_utils.h>
#include <versionbits.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <ranges>
#include <vector>

using namespace util::hex_literals;

namespace {
FastRandomContext& Rng()
{
    static FastRandomContext g{/*fDeterministic=*/true};
    return g;
}

size_t GeomCount(double thresh_prob)
{
    size_t n{1};
    while (Rng().randrange<uint8_t>(100) < thresh_prob * 100) {
        ++n;
    }
    return n;
}


CKey RandKey()
{
    CKey k;
    const auto s{Rng().rand256()};
    k.Set(s.begin(), s.end(), /*fCompressedIn=*/true);
    return k;
}

CPubKey RandPub()
{
    const auto key{RandKey()};
    return key.GetPubKey();
}

std::array<std::pair<double, std::function<CScript()>>, 11> createTable(const benchmark::ScriptRecipe& rec)
{
    std::array<std::pair<double, std::function<CScript()>>, 11> table{
        std::pair{rec.anchor_prob, [&] { return GetScriptForDestination(PayToAnchor{}); }},
        std::pair{rec.multisig_prob, [&] {
            const size_t nRequired{1 + GeomCount(rec.geometric_base_prob)};
            std::vector<CPubKey> keys;
            keys.reserve(nRequired);
            for (size_t i{0}; i < nRequired; ++i) keys.emplace_back(RandPub());
            return GetScriptForMultisig(nRequired, keys);
        }},
        std::pair{rec.null_data_prob, [&] { return CScript() << OP_RETURN << "00"_hex; }},
        std::pair{rec.pubkey_prob, [&] { return GetScriptForRawPubKey(RandPub()); }},
        std::pair{rec.pubkeyhash_prob, [&] { return GetScriptForDestination(PKHash(RandPub())); }},
        std::pair{rec.scripthash_prob, [&] { return GetScriptForDestination(ScriptHash(CScript() << OP_TRUE)); }},
        std::pair{rec.witness_v1_taproot_prob, [&] { return GetScriptForDestination(WitnessV1Taproot(XOnlyPubKey(RandPub()))); }},
        std::pair{rec.witness_v0_keyhash_prob, [&] { return GetScriptForDestination(WitnessV0KeyHash(RandPub())); }},
        std::pair{rec.witness_v0_scripthash_prob, [&] { return GetScriptForDestination(WitnessV0ScriptHash(CScript() << OP_TRUE)); }},
        std::pair{rec.nonstandard_prob, [&] { return CScript() << OP_RETURN << OP_CHECKSIG; }},
        std::pair{rec.witness_unknown_prob, [&] { return CScript() << OP_2 << Rng().randbytes<uint8_t>(32); }},
    };

    double sum{};
    for (const auto& p : table | std::views::keys) sum += p;
    assert(sum <= 1);

    return table;
}

/* ===================================================================== */
/*  block builder                                                        */
/* ===================================================================== */
CBlock BuildBlock(const std::unique_ptr<const CChainParams>& chainParams, const benchmark::ScriptRecipe& rec)
{
    ECC_Context ecc_context{};

    assert(rec.geometric_base_prob >= 0 && rec.geometric_base_prob <= 1);
    const auto tx_count{rec.tx_count ? rec.tx_count : 1000 + Rng().randrange(2000)};

    CBlock blk{};
    blk.vtx.reserve(1 + tx_count);

    // coinbase
    {
        CMutableTransaction cb;
        cb.vin = {CTxIn(COutPoint())};
        cb.vin[0].scriptSig = CScript() << 0 << OP_0;
        cb.vout = {CTxOut(50 * COIN, CScript() << OP_TRUE)};
        blk.vtx.push_back(MakeTransactionRef(std::move(cb)));
    }

    auto table{createTable(rec)};
    auto rand_script{[&] {
        double x{Rng().rand64() * (1.0 / std::numeric_limits<uint64_t>::max())};
        for (const auto& [p, make] : table) {
            if (x < p) return make();
            x -= p;
        }
        const auto raw{Rng().randbytes<uint8_t>(Rng().randrange(100))};
        return CScript(raw.begin(), raw.end());
    }};

    for (size_t i{0}; i < tx_count; ++i) {
        CMutableTransaction tx;
        const size_t in_count{GeomCount(rec.geometric_base_prob)};
        tx.vin.resize(in_count);
        for (size_t in{0}; in < in_count; ++in) {
            tx.vin[in].prevout = COutPoint{Txid::FromUint256(Rng().rand256()), uint32_t(GeomCount(rec.geometric_base_prob))};
            tx.vin[in].scriptSig = rand_script();
            tx.vin[in].nSequence = CTxIn::SEQUENCE_FINAL;
        }

        const size_t out_count{GeomCount(rec.geometric_base_prob)};
        tx.vout.resize(out_count);
        for (size_t out{0}; out < out_count; ++out)
            tx.vout[out] = CTxOut{1 * COIN, rand_script()};

        blk.vtx.push_back(MakeTransactionRef(std::move(tx)));
    }

    blk.nVersion = VERSIONBITS_LAST_OLD_BLOCK_VERSION;
    blk.nTime = chainParams->GenesisBlock().nTime;
    blk.hashPrevBlock.SetNull();
    blk.nBits = UintToArith256(chainParams->GetConsensus().powLimit).GetCompact();   // lowest target
    blk.nNonce = 0;
    blk.hashMerkleRoot = BlockMerkleRoot(blk);

    while (!CheckProofOfWork(blk.GetHash(), blk.nBits, chainParams->GetConsensus())) {
        ++blk.nNonce;
    }
    return blk;
}

DataStream SerializeBlock(const CBlock& blk)
{
    DataStream ds;
    ds << TX_WITH_WITNESS(blk);
    return ds;
}
} // namespace

namespace benchmark {
DataStream GetCustomBlockData(const std::unique_ptr<const CChainParams>& chainParams, const ScriptRecipe& recipe)
{
    return SerializeBlock(BuildBlock(chainParams, recipe));
}

CBlock GetCustomBlock(const std::unique_ptr<const CChainParams>& chainParams, const ScriptRecipe& recipe)
{
    return BuildBlock(chainParams, recipe);
}
} // namespace benchmark
