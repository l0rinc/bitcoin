// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <common/system.h>
#include <inputfetcher.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <primitives/transaction_identifier.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <set>
#include <stdexcept>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(inputfetcher_tests)

    struct InputFetcherTest : BasicTestingSetup
    {
        std::unique_ptr<InputFetcher> m_fetcher{nullptr};
        std::unique_ptr<CBlock> m_block{nullptr};

        CBlock CreateBlock(int32_t num_txs)
        {
            CBlock block;
            CMutableTransaction coinbase;
            coinbase.vin.emplace_back();
            block.vtx.push_back(MakeTransactionRef(coinbase));

            Txid prevhash{Txid::FromUint256(m_rng.rand256())};
            for (auto i{1}; i < num_txs; ++i) {
                CMutableTransaction tx;
                const auto txid{m_rng.randbool() ? Txid::FromUint256(uint256(i)) : prevhash}; // TODO this means the spent is never on the same thread?
                tx.vin.emplace_back(COutPoint(txid, m_rng.randbool() ? 0 : m_rng.rand32()));
                if (m_rng.randbool()) {
                    prevhash = tx.GetHash(); // TODO This can theoretically simulate double spends
                }
                block.vtx.push_back(MakeTransactionRef(tx));
            }

            return block;
        }

        explicit InputFetcherTest(const ChainType chainType = ChainType::MAIN, const TestOpts& opts = {})
            : BasicTestingSetup{chainType, opts}
        {
            SeedRandomForTest(SeedRand::FIXED_SEED);

            const auto cores{GetNumCores()};
            const auto num_txs{m_rng.randrange(cores * 10)};
            m_block = std::make_unique<CBlock>(CreateBlock(num_txs));
            const auto worker_threads{m_rng.randrange(cores * 2) + 1};
            m_fetcher = std::make_unique<InputFetcher>(worker_threads);
        }

        InputFetcher& getFetcher() const { return *m_fetcher; }
        const CBlock& getBlock() const { return *m_block; }
    };

    BOOST_FIXTURE_TEST_CASE(fetch_inputs, InputFetcherTest)
    {
        const auto& block{getBlock()};

        CCoinsView dummy;
        CCoinsViewCache db(&dummy);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                auto outpoint{in.prevout};
                Coin coin{};
                coin.out.nValue = 1;
                db.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }

        CCoinsViewCache cache(&db);
        CCoinsViewCache block_cache(&cache);
        getFetcher().FetchInputs(cache, block_cache, db, block);

        std::set<Txid> txids; // sorted set to minimize duplication with impl
        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) {
                BOOST_CHECK(!cache.HaveCoinInCache(tx->vin[0].prevout));
                BOOST_CHECK(!block_cache.HaveCoinInCache(tx->vin[0].prevout));
            } else {
                for (const auto& in : tx->vin) {
                    BOOST_CHECK(!db.AccessCoin(in.prevout).IsSpent());
                    BOOST_CHECK(!cache.HaveCoinInCache(in.prevout));
                    BOOST_CHECK(block_cache.HaveCoinInCache(in.prevout) || txids.contains(in.prevout.hash));
                }
                for (uint32_t o{0}; o < tx->vout.size(); ++o) {
                    BOOST_CHECK(!cache.HaveCoinInCache(COutPoint{tx->GetHash(), o}));
                    BOOST_CHECK(!block_cache.HaveCoinInCache(COutPoint{tx->GetHash(), o}));
                }
                txids.insert(tx->GetHash());
            }
        }
    }

    // Test for the case where a block spends coins that are spent in the cache, but
    // the spentness has not been flushed to the db. So the input fetcher will fetch
    // the coin from the db since HaveCoinInCache will return false for an existing
    // but spent coin. However, the fetched coin will fail to be inserted into the
    // cache because the emplace call in EmplaceCoinInternalDANGER will not insert
    // the unspent coin due to the collision with the already spent coin in the map.
    BOOST_FIXTURE_TEST_CASE(fetch_no_double_spend, InputFetcherTest)
    {
        const auto& block{getBlock()};
        CCoinsView dummy;
        CCoinsViewCache db(&dummy);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                auto outpoint{in.prevout};
                Coin coin{};
                coin.out.nValue = 1;
                db.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }

        CCoinsViewCache cache(&db);

        // Add all inputs as spent already in cache
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                auto outpoint{in.prevout};
                Coin coin{};
                assert(coin.IsSpent());
                cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }

        CCoinsViewCache block_cache(&cache);
        getFetcher().FetchInputs(cache, block_cache, db, block);

        // Coins are still spent, even though they exist unspent in the parent db
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!cache.HaveCoinInCache(in.prevout));
            }
        }
    }

    BOOST_FIXTURE_TEST_CASE(fetch_no_inputs, InputFetcherTest)
    {
        const auto& block{getBlock()};
        CCoinsView db;
        CCoinsViewCache cache(&db);
        CCoinsViewCache block_cache(&cache);
        getFetcher().FetchInputs(cache, block_cache, db, block);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!block_cache.HaveCoinInCache(in.prevout));
            }
        }
    }

    class ThrowCoinsView : public CCoinsView
    {
        std::optional<Coin> GetCoin(const COutPoint&) const override
        {
            throw std::runtime_error("database error");
        }
    };

    BOOST_FIXTURE_TEST_CASE(fetch_input_exceptions, InputFetcherTest)
    {
        const auto& block{getBlock()};
        ThrowCoinsView db;
        CCoinsViewCache cache(&db);
        CCoinsViewCache block_cache(&cache);
        getFetcher().FetchInputs(cache, block_cache, db, block);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!block_cache.HaveCoinInCache(in.prevout));
            }
        }
    }

BOOST_AUTO_TEST_SUITE_END()
