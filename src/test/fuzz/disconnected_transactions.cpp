// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/disconnected_transactions.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>

#include <cassert>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace {
CTransactionRef MakeTransaction(FuzzedDataProvider& fuzzed_data_provider)
{
    CMutableTransaction tx;
    tx.version = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    tx.nLockTime = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    return MakeTransactionRef(std::move(tx));
}

void AssertPoolMatches(const DisconnectedBlockTransactions& pool, const std::set<Txid>& expected)
{
    assert(pool.size() == expected.size());
}
} // namespace

FUZZ_TARGET(disconnected_transactions)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    DisconnectedBlockTransactions pool{MAX_DISCONNECTED_TX_POOL_BYTES};
    std::vector<CTransactionRef> known;
    std::set<Txid> expected;

    const auto add = [&](const CTransactionRef& tx) {
        const auto evicted{pool.AddTransactionsFromBlock({tx})};
        assert(evicted.empty());
        expected.insert(tx->GetHash());
        AssertPoolMatches(pool, expected);
    };

    // Force the historical cross-block duplicate transition even for the empty seed.
    CMutableTransaction first_mut;
    first_mut.vin.emplace_back();
    const CTransactionRef first{MakeTransactionRef(first_mut)};
    known.push_back(first);
    add(first);
    add(first);

    LIMITED_WHILE(fuzzed_data_provider.remaining_bytes() > 0, 64)
    {
        if (fuzzed_data_provider.ConsumeBool() || known.empty()) {
            const CTransactionRef tx{MakeTransaction(fuzzed_data_provider)};
            known.push_back(tx);
            add(tx);
        } else {
            const CTransactionRef tx{PickValue(fuzzed_data_provider, known)};
            std::vector<CTransactionRef> block{tx};
            if (fuzzed_data_provider.ConsumeBool()) block.push_back(tx);
            pool.removeForBlock(block);
            expected.erase(tx->GetHash());
            AssertPoolMatches(pool, expected);
        }

        if (fuzzed_data_provider.ConsumeBool()) {
            const auto queued{pool.take()};
            std::set<Txid> taken;
            for (const auto& tx : queued) {
                assert(taken.insert(tx->GetHash()).second);
            }
            assert(taken == expected);
            expected.clear();
            AssertPoolMatches(pool, expected);
        }
    }

    const auto queued{pool.take()};
    std::set<Txid> taken;
    for (const auto& tx : queued) {
        assert(taken.insert(tx->GetHash()).second);
    }
    assert(taken == expected);
}
