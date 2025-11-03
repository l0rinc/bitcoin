// Copyright (c) The Bitcoin Core developers
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

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_set>

BOOST_AUTO_TEST_SUITE(inputfetcher_tests)

struct InputFetcherTest : BasicTestingSetup {
private:
    std::unique_ptr<InputFetcher> m_fetcher{nullptr};
    std::unique_ptr<CBlock> m_block{nullptr};

    CBlock CreateBlock(int32_t num_txs)
    {
        CBlock block;
        CMutableTransaction coinbase;
        coinbase.vin.emplace_back();
        block.vtx.push_back(MakeTransactionRef(coinbase));

        Txid prevhash{Txid::FromUint256(uint256(1))};

        for (auto i{1}; i < num_txs; ++i) {
            CMutableTransaction tx;
            const auto txid{m_rng.randbool() ? Txid::FromUint256(uint256(i)) : prevhash};
            tx.vin.emplace_back(COutPoint(txid, 0));
            prevhash = tx.GetHash();
            block.vtx.push_back(MakeTransactionRef(tx));
        }

        return block;
    }

public:
    explicit InputFetcherTest(const ChainType chainType = ChainType::MAIN,
                              TestOpts opts = {})
        : BasicTestingSetup{chainType, opts}
    {
        SeedRandomForTest(SeedRand::FIXED_SEED);

        const auto cores{GetNumCores()};
        const auto num_txs{m_rng.randrange(cores * 10)};
        m_block = std::make_unique<CBlock>(CreateBlock(num_txs));
        const auto worker_threads{m_rng.randrange(cores * 2) + 1};
        m_fetcher = std::make_unique<InputFetcher>(worker_threads);
    }

    InputFetcher& getFetcher() { return *m_fetcher; }
    const CBlock& getBlock() { return *m_block; }
};

BOOST_FIXTURE_TEST_CASE(fetch_inputs_from_db, InputFetcherTest)
{
    const auto& block{getBlock()};
    for (auto i{0}; i < 3; ++i) {
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

        CCoinsViewCache main_cache(&db);
        CCoinsViewCache cache(&main_cache);
        getFetcher().FetchInputs(cache, main_cache, db, block);

        std::unordered_set<Txid, SaltedTxidHasher> txids{};
        txids.reserve(block.vtx.size() - 1);

        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) {
                BOOST_CHECK(!cache.HaveCoinInCache(tx->vin[0].prevout));
            } else {
                for (const auto& in : tx->vin) {
                    const auto& outpoint{in.prevout};
                    const auto have{cache.HaveCoinInCache(outpoint)};
                    const auto should_have{!txids.contains(outpoint.hash)};
                    BOOST_CHECK(should_have ? have : !have);
                }
                txids.emplace(tx->GetHash());
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(fetch_inputs_from_cache, InputFetcherTest)
{
    const auto& block{getBlock()};
    for (auto i{0}; i < 3; ++i) {
        CCoinsView dummy;
        CCoinsViewCache main_cache(&dummy);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                auto outpoint{in.prevout};
                Coin coin{};
                coin.out.nValue = 1;
                main_cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }

        CCoinsViewCache cache(&main_cache);
        getFetcher().FetchInputs(cache, main_cache, dummy, block);

        std::unordered_set<Txid, SaltedTxidHasher> txids{};
        txids.reserve(block.vtx.size() - 1);

        for (const auto& tx : block.vtx) {
            if (tx->IsCoinBase()) {
                BOOST_CHECK(!cache.HaveCoinInCache(tx->vin[0].prevout));
            } else {
                for (const auto& in : tx->vin) {
                    const auto& outpoint{in.prevout};
                    const auto have{cache.HaveCoinInCache(outpoint)};
                    const auto should_have{!txids.contains(outpoint.hash)};
                    BOOST_CHECK(should_have ? have : !have);
                }
                txids.emplace(tx->GetHash());
            }
        }
    }
}

// Test for the case where a block spends coins that are spent in the cache, but
// the spentness has not been flushed to the db.
BOOST_FIXTURE_TEST_CASE(fetch_no_double_spend, InputFetcherTest)
{
    const auto& block{getBlock()};
    for (auto i{0}; i < 3; ++i) {
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

        CCoinsViewCache main_cache(&db);

        // Add all inputs as spent already in cache
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                auto outpoint{in.prevout};
                Coin coin{};
                assert(coin.IsSpent());
                main_cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }

        CCoinsViewCache cache(&main_cache);
        getFetcher().FetchInputs(cache, main_cache, db, block);

        // Coins are not added to the temp cache, even though they exist unspent in the parent db
        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!cache.GetPossiblySpentCoinFromCache(in.prevout));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(fetch_no_inputs, InputFetcherTest)
{
    const auto& block{getBlock()};
    for (auto i{0}; i < 3; ++i) {
        CCoinsView db;
        CCoinsViewCache main_cache(&db);
        CCoinsViewCache cache(&main_cache);
        getFetcher().FetchInputs(cache, main_cache, db, block);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!cache.GetPossiblySpentCoinFromCache(in.prevout));
            }
        }
    }
}

struct ThrowCoinsView : CCoinsView {
    std::optional<Coin> GetCoin(const COutPoint&) const override
    {
        throw std::runtime_error("database error");
    }
};

BOOST_FIXTURE_TEST_CASE(fetch_input_exceptions, InputFetcherTest)
{
    const auto& block{getBlock()};
    for (auto i{0}; i < 3; ++i) {
        ThrowCoinsView db;
        CCoinsViewCache main_cache(&db);
        CCoinsViewCache cache(&main_cache);
        getFetcher().FetchInputs(cache, main_cache, db, block);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!cache.GetPossiblySpentCoinFromCache(in.prevout));
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(fetch_with_zero_workers, InputFetcherTest)
{
    const auto& block{getBlock()};
    for (auto i{0}; i < 3; ++i) {
        CCoinsView db;
        CCoinsViewCache main_cache(&db);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                auto outpoint{in.prevout};
                Coin coin{};
                coin.out.nValue = 1;
                main_cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }

        CCoinsViewCache cache(&main_cache);
        InputFetcher fetcher{0};
        fetcher.FetchInputs(cache, main_cache, db, block);

        for (const auto& tx : block.vtx) {
            for (const auto& in : tx->vin) {
                BOOST_CHECK(!cache.GetPossiblySpentCoinFromCache(in.prevout));
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
