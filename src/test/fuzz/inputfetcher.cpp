// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <inputfetcher.h>
#include <primitives/transaction_identifier.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>

#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>

using DbMap = std::map<const COutPoint, std::pair<std::optional<const Coin>, bool>>;

struct DbCoinsView : CCoinsView {
    DbMap& m_map;
    DbCoinsView(DbMap& map) noexcept : m_map(map) {}

    std::optional<Coin> GetCoin(const COutPoint& outpoint) const override
    {
        const auto it{m_map.find(outpoint)};
        assert(it != m_map.end());
        const auto [coin, err] = it->second;
        if (err) {
            throw std::runtime_error("database error");
        }
        return coin;
    }
};

struct NoAccessCoinsView : CCoinsView {
    std::optional<Coin> GetCoin(const COutPoint&) const override { abort(); }
};

std::optional<InputFetcher> g_fetcher{};

static void setup_threadpool_test()
{
    LogInstance().DisableLogging();
    g_fetcher.emplace(3);
}

FUZZ_TARGET(inputfetcher, .init = setup_threadpool_test)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        CBlock block;
        Txid prevhash{Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider))};

        DbMap db_map{};
        std::map<const COutPoint, const Coin> cache_map{};

        DbCoinsView db(db_map);

        NoAccessCoinsView back;
        CCoinsViewCache main_cache(&back);

        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
        {
            CMutableTransaction tx;

            LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10)
            {
                const auto txid{fuzzed_data_provider.ConsumeBool() ? Txid::FromUint256(ConsumeUInt256(fuzzed_data_provider)) : prevhash};
                const auto index{fuzzed_data_provider.ConsumeIntegral<uint32_t>()};
                const COutPoint outpoint(txid, index);

                tx.vin.emplace_back(outpoint);

                std::optional<Coin> maybe_coin;
                if (fuzzed_data_provider.ConsumeBool()) {
                    Coin coin{};
                    coin.fCoinBase = fuzzed_data_provider.ConsumeBool();
                    coin.nHeight =
                        fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                            0, std::numeric_limits<int32_t>::max());
                    coin.out.nValue = ConsumeMoney(fuzzed_data_provider);
                    assert(!coin.IsSpent());
                    maybe_coin = coin;
                } else {
                    maybe_coin = std::nullopt;
                }
                db_map.try_emplace(outpoint, std::make_pair(
                                                 maybe_coin,
                                                 fuzzed_data_provider.ConsumeBool()));

                // Add the coin to the cache
                if (fuzzed_data_provider.ConsumeBool()) {
                    Coin coin{};
                    coin.fCoinBase = fuzzed_data_provider.ConsumeBool();
                    coin.nHeight =
                        fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(
                            0, std::numeric_limits<int32_t>::max());
                    coin.out.nValue =
                        fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(
                            -1, MAX_MONEY);
                    cache_map.try_emplace(outpoint, coin);
                    main_cache.EmplaceCoinInternalDANGER(
                        COutPoint(outpoint),
                        std::move(coin));
                }
            }

            prevhash = tx.GetHash();
            block.vtx.push_back(MakeTransactionRef(tx));
        }

        CCoinsViewCache cache(&back);
        g_fetcher->FetchInputs(cache, main_cache, db, block);

        Coin empty_coin;
        for (const auto& [outpoint, pair] : db_map) {
            const auto coin{cache.GetPossiblySpentCoinFromCache(outpoint)};
            if (!coin) {
                continue;
            }
            // No spent coins should be inserted into the cache
            assert(!coin->IsSpent());

            // Check coins in the main_cache were inserted instead of db
            const auto it{cache_map.find(outpoint)};
            if (it != cache_map.end()) {
                const auto& cache_coin{it->second};
                assert(!cache_coin.IsSpent());
                assert(coin->fCoinBase == cache_coin.fCoinBase);
                assert(coin->nHeight == cache_coin.nHeight);
                assert(coin->out == cache_coin.out);
                continue;
            }

            // Check any newly added coins in the cache are the same as the db
            const auto& [maybe_coin, err] = pair;
            assert(maybe_coin && !err);
            assert(coin->fCoinBase == maybe_coin->fCoinBase);
            assert(coin->nHeight == maybe_coin->nHeight);
            assert(coin->out == maybe_coin->out);
        }
    }
}
