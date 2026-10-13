// Copyright (c) 2016-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <coins.h>
#include <consensus/amount.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <key.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <test/util/transaction_utils.h>
#include <util/check.h>

#include <array>
#include <cstdint>
#include <span>
#include <vector>

// Microbenchmark for simple accesses to a CCoinsViewCache database. Note from
// laanwj, "replicating the actual usage patterns of the client is hard though,
// many times micro-benchmarks of the database showed completely different
// characteristics than e.g. reindex timings. But that's not a requirement of
// every benchmark."
// (https://github.com/bitcoin/bitcoin/issues/7883#issuecomment-224807484)
static void CCoinsCaching(benchmark::Bench& bench)
{
    ECC_Context ecc_context{};

    FillableSigningProvider keystore;
    CCoinsViewCache coins{&CoinsViewEmpty::Get()};
    std::vector<CMutableTransaction> dummyTransactions =
        SetupDummyInputs(keystore, coins, {11 * COIN, 50 * COIN, 21 * COIN, 22 * COIN});

    CMutableTransaction t1;
    t1.vin.resize(3);
    t1.vin[0].prevout.hash = dummyTransactions[0].GetHash();
    t1.vin[0].prevout.n = 1;
    t1.vin[0].scriptSig << std::vector<unsigned char>(65, 0);
    t1.vin[1].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[1].prevout.n = 0;
    t1.vin[1].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vin[2].prevout.hash = dummyTransactions[1].GetHash();
    t1.vin[2].prevout.n = 1;
    t1.vin[2].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4);
    t1.vout.resize(2);
    t1.vout[0].nValue = 90 * COIN;
    t1.vout[0].scriptPubKey << OP_1;

    // Benchmark.
    const CTransaction tx_1(t1);
    bench.run([&] {
        assert(ValidateInputsStandardness(tx_1, coins).IsValid());
    });
}

BENCHMARK(CCoinsCaching);

// Account for sigops with cached prevouts, without executing signatures.
static void WitnessSigOpsCachedInputs(benchmark::Bench& bench)
{
    constexpr size_t NUM_INPUTS{4096};
    CCoinsViewCache coins{&CoinsViewEmpty::Get(), /*deterministic=*/true};
    CMutableTransaction spending;
    for (size_t i{0}; i < NUM_INPUTS; ++i) {
        const COutPoint outpoint{Txid::FromUint256(uint256::ONE), static_cast<uint32_t>(i)};
        spending.vin.emplace_back(outpoint);
        const CScript script{i % 2 == 0 ? CScript{} << OP_0 << std::vector<unsigned char>(20, 1)
                                       : CScript{} << OP_1 << std::vector<unsigned char>(32, 1)};
        coins.AddCoin(outpoint, Coin{CTxOut{1, script}, 1, false}, false);
    }
    const CTransaction tx{spending};
    bench.batch(NUM_INPUTS).unit("input").run([&] {
        const auto count{GetTransactionSigOpCost(tx, coins, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS)};
        ankerl::nanobench::doNotOptimizeAway(count);
        assert(count == NUM_INPUTS / 2);
    });
}

BENCHMARK(WitnessSigOpsCachedInputs);
