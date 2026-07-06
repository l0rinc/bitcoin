// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/check.h>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

bool CastToBool(const std::vector<unsigned char>& vch);

namespace {
int32_t CanonicalSigHashCacheAlias(int32_t hash_type)
{
    const int32_t any_one_can_pay{hash_type & SIGHASH_ANYONECANPAY};
    switch (hash_type & 0x1f) {
    case SIGHASH_NONE:
        return any_one_can_pay | SIGHASH_NONE;
    case SIGHASH_SINGLE:
        return any_one_can_pay | SIGHASH_SINGLE;
    default:
        return any_one_can_pay | SIGHASH_ALL;
    }
}

void AssertSigHashCacheAliases(const CScript& scriptcode, int32_t hash_type, uint64_t marker)
{
    SigHashCache cache;
    HashWriter stored_writer;
    stored_writer << marker;
    HashWriter expected_writer;
    expected_writer << marker;
    const uint256 expected_hash{expected_writer.GetHash()};

    cache.Store(hash_type, scriptcode, stored_writer);

    auto assert_loads = [&](int32_t alias) {
        HashWriter loaded_writer;
        Assert(cache.Load(alias, scriptcode, loaded_writer));
        Assert(loaded_writer.GetHash() == expected_hash);
    };

    const int32_t canonical_alias{CanonicalSigHashCacheAlias(hash_type)};
    assert_loads(canonical_alias);
    assert_loads(canonical_alias | (hash_type & ~int32_t{0xff}));

    CScript other_scriptcode{scriptcode};
    other_scriptcode << OP_CODESEPARATOR;
    HashWriter missed_writer;
    Assert(!cache.Load(canonical_alias, other_scriptcode, missed_writer));

    const int32_t any_one_can_pay{canonical_alias & SIGHASH_ANYONECANPAY};
    const int32_t different_mode{
        any_one_can_pay | (((canonical_alias & 0x1f) == SIGHASH_NONE) ? SIGHASH_SINGLE : SIGHASH_NONE)};
    Assert(!cache.Load(different_mode, scriptcode, missed_writer));
}
} // namespace

FUZZ_TARGET(script_interpreter)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    {
        const CScript script_code = ConsumeScript(fuzzed_data_provider);
        const std::optional<CMutableTransaction> mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
        if (mtx) {
            const CTransaction tx_to{*mtx};
            const unsigned int in = fuzzed_data_provider.ConsumeIntegral<unsigned int>();
            if (in < tx_to.vin.size()) {
                auto n_hash_type = fuzzed_data_provider.ConsumeIntegral<int>();
                auto amount = ConsumeMoney(fuzzed_data_provider);
                auto sigversion = fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0});
                (void)SignatureHash(script_code, tx_to, in, n_hash_type, amount, sigversion, nullptr);
                const std::optional<CMutableTransaction> mtx_precomputed = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                if (mtx_precomputed) {
                    const CTransaction tx_precomputed{*mtx_precomputed};
                    const PrecomputedTransactionData precomputed_transaction_data{tx_precomputed};
                    n_hash_type = fuzzed_data_provider.ConsumeIntegral<int>();
                    amount = ConsumeMoney(fuzzed_data_provider);
                    sigversion = fuzzed_data_provider.PickValueInArray({SigVersion::BASE, SigVersion::WITNESS_V0});
                    (void)SignatureHash(script_code, tx_to, in, n_hash_type, amount, sigversion, &precomputed_transaction_data);
                }
            }
        }
    }
    {
        (void)CastToBool(ConsumeRandomLengthByteVector(fuzzed_data_provider));
    }
}

/** Differential fuzzing for SignatureHash with and without cache. */
FUZZ_TARGET(sighash_cache)
{
    FuzzedDataProvider provider(buffer.data(), buffer.size());

    // Get inputs to the sighash function that won't change across types.
    const auto scriptcode{ConsumeScript(provider)};
    const auto stale_tx{ConsumeTransaction(provider, std::nullopt)};
    const auto tx{ConsumeTransaction(provider, std::nullopt)};
    if (tx.vin.empty()) return;
    const auto in_index{provider.ConsumeIntegralInRange<uint32_t>(0, tx.vin.size() - 1)};
    const auto amount{ConsumeMoney(provider)};
    const auto sigversion{(SigVersion)provider.ConsumeIntegralInRange(0, 1)};

    // Check the sighash function will give the same result for 100 fuzzer-generated hash types whether or not a cache is
    // provided. The cache is conserved across types to exercise cache hits.
    SigHashCache sighash_cache{};
    for (int i{0}; i < 100; ++i) {
        const auto hash_type{((i & 2) == 0) ? provider.ConsumeIntegral<int8_t>() : provider.ConsumeIntegral<int32_t>()};
        AssertSigHashCacheAliases(scriptcode, hash_type, provider.ConsumeIntegral<uint64_t>());
        const auto nocache_res{SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion)};
        const auto cache_res{SignatureHash(scriptcode, tx, in_index, hash_type, amount, sigversion, nullptr, &sighash_cache)};
        Assert(nocache_res == cache_res);
    }

    PrecomputedTransactionData reused_txdata;
    reused_txdata.Init(stale_tx, /*spent_outputs=*/{}, /*force=*/true);
    reused_txdata.Init(tx, /*spent_outputs=*/{}, /*force=*/false);
    const auto hash_type{provider.ConsumeIntegral<int32_t>()};
    Assert(SignatureHash(scriptcode, tx, in_index, hash_type, amount, SigVersion::WITNESS_V0) ==
           SignatureHash(scriptcode, tx, in_index, hash_type, amount, SigVersion::WITNESS_V0, &reused_txdata));
}
