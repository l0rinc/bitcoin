// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <crypto/sha256.h>
#include <kernel/chainstatemanager_opts.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <util/threadpool.h>

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace {

/** Number of distinct COutPoint values used in this test. */
constexpr uint32_t NUM_OUTPOINTS = 256;
/** Number of distinct Coin values used in this test (ignoring nHeight). */
constexpr uint32_t NUM_COINS = 256;
/** Number of distinct best block hashes used in this test. */
constexpr uint32_t NUM_BLOCK_HASHES = 256;
/** Maximum number CCoinsViewCache objects used in this test. */
constexpr uint32_t MAX_CACHES = 4;
/** Data type large enough to hold NUM_COINS-1. */
using coinidx_type = uint8_t;

struct PrecomputedData
{
    //! Randomly generated COutPoint values.
    COutPoint outpoints[NUM_OUTPOINTS];

    //! Randomly generated Coin values.
    Coin coins[NUM_COINS];

    //! Block with a tx containing as inputs the above outpoints.
    CBlock block;

    //! Randomly generated best block hashes.
    uint256 block_hashes[NUM_BLOCK_HASHES];

    PrecomputedData()
    {
        static const uint8_t PREFIX_O[1] = {'o'}; /** Hash prefix for outpoint hashes. */
        static const uint8_t PREFIX_S[1] = {'s'}; /** Hash prefix for coins scriptPubKeys. */
        static const uint8_t PREFIX_M[1] = {'m'}; /** Hash prefix for coins nValue/fCoinBase. */
        static const uint8_t PREFIX_B[1] = {'b'}; /** Hash prefix for best block hashes. */

        CMutableTransaction coinbase;
        coinbase.vin.emplace_back();
        block.vtx.push_back(MakeTransactionRef(coinbase));

        CMutableTransaction tx;
        for (uint32_t i = 0; i < NUM_OUTPOINTS; ++i) {
            uint32_t idx = (i * 1200U) >> 12; /* Map 3 or 4 entries to same txid. */
            const uint8_t ser[4] = {uint8_t(idx), uint8_t(idx >> 8), uint8_t(idx >> 16), uint8_t(idx >> 24)};
            uint256 txid;
            CSHA256().Write(PREFIX_O, 1).Write(ser, sizeof(ser)).Finalize(txid.begin());
            outpoints[i].hash = Txid::FromUint256(txid);
            outpoints[i].n = i;
            tx.vin.emplace_back(outpoints[i]);
        }
        block.vtx.push_back(MakeTransactionRef(tx));

        for (uint32_t i = 0; i < NUM_COINS; ++i) {
            const uint8_t ser[4] = {uint8_t(i), uint8_t(i >> 8), uint8_t(i >> 16), uint8_t(i >> 24)};
            uint256 hash;
            CSHA256().Write(PREFIX_S, 1).Write(ser, sizeof(ser)).Finalize(hash.begin());
            /* Convert hash to scriptPubkeys (of different lengths, so SanityCheck's cached memory
             * usage check has a chance to detect mismatches). */
            switch (i % 5U) {
            case 0: /* P2PKH */
                coins[i].out.scriptPubKey.resize(25);
                coins[i].out.scriptPubKey[0] = OP_DUP;
                coins[i].out.scriptPubKey[1] = OP_HASH160;
                coins[i].out.scriptPubKey[2] = 20;
                std::copy(hash.begin(), hash.begin() + 20, coins[i].out.scriptPubKey.begin() + 3);
                coins[i].out.scriptPubKey[23] = OP_EQUALVERIFY;
                coins[i].out.scriptPubKey[24] = OP_CHECKSIG;
                break;
            case 1: /* P2SH */
                coins[i].out.scriptPubKey.resize(23);
                coins[i].out.scriptPubKey[0] = OP_HASH160;
                coins[i].out.scriptPubKey[1] = 20;
                std::copy(hash.begin(), hash.begin() + 20, coins[i].out.scriptPubKey.begin() + 2);
                coins[i].out.scriptPubKey[22] = OP_EQUAL;
                break;
            case 2: /* P2WPKH */
                coins[i].out.scriptPubKey.resize(22);
                coins[i].out.scriptPubKey[0] = OP_0;
                coins[i].out.scriptPubKey[1] = 20;
                std::copy(hash.begin(), hash.begin() + 20, coins[i].out.scriptPubKey.begin() + 2);
                break;
            case 3: /* P2WSH */
                coins[i].out.scriptPubKey.resize(34);
                coins[i].out.scriptPubKey[0] = OP_0;
                coins[i].out.scriptPubKey[1] = 32;
                std::copy(hash.begin(), hash.begin() + 32, coins[i].out.scriptPubKey.begin() + 2);
                break;
            case 4: /* P2TR */
                coins[i].out.scriptPubKey.resize(34);
                coins[i].out.scriptPubKey[0] = OP_1;
                coins[i].out.scriptPubKey[1] = 32;
                std::copy(hash.begin(), hash.begin() + 32, coins[i].out.scriptPubKey.begin() + 2);
                break;
            }
            /* Hash again to construct nValue and fCoinBase. */
            CSHA256().Write(PREFIX_M, 1).Write(ser, sizeof(ser)).Finalize(hash.begin());
            coins[i].out.nValue = CAmount(hash.GetUint64(0) % MAX_MONEY);
            coins[i].fCoinBase = (hash.GetUint64(1) & 7) == 0;
            coins[i].nHeight = 0; /* Real nHeight used in simulation is set dynamically. */
        }

        for (uint32_t i = 0; i < NUM_BLOCK_HASHES; ++i) {
            const uint8_t ser[4] = {uint8_t(i), uint8_t(i >> 8), uint8_t(i >> 16), uint8_t(i >> 24)};
            CSHA256().Write(PREFIX_B, 1).Write(ser, sizeof(ser)).Finalize(block_hashes[i].begin());
        }
    }
};

enum class EntryType : uint8_t
{
    /* This entry in the cache does not exist (so we'd have to look in the parent cache). */
    NONE,

    /* This entry in the cache corresponds to an unspent coin. */
    UNSPENT,

    /* This entry in the cache corresponds to a spent coin. */
    SPENT,
};

struct CacheEntry
{
    /* Type of entry. */
    EntryType entrytype;

    /* Index in the coins array this entry corresponds to (only if entrytype == UNSPENT). */
    coinidx_type coinidx;

    /* nHeight value for this entry (so the coins[coinidx].nHeight value is ignored; only if entrytype == UNSPENT). */
    uint32_t height;
};

struct CacheLevel
{
    CacheEntry entry[NUM_OUTPOINTS];

    void Wipe() {
        for (uint32_t i = 0; i < NUM_OUTPOINTS; ++i) {
            entry[i].entrytype = EntryType::NONE;
        }
    }
};

/** Class for the base of the hierarchy (roughly simulating a memory-backed CCoinsViewDB).
 *
 * The initial state consists of the empty UTXO set.
 */
class CoinsViewBottom final : public CoinsViewEmpty
{
    std::map<COutPoint, Coin> m_data;
    uint256 m_best_block;

public:
    std::optional<Coin> GetCoin(const COutPoint& outpoint) const final
    {
        if (auto it{m_data.find(outpoint)}; it != m_data.end()) {
            assert(!it->second.IsSpent());
            return it->second;
        }
        return std::nullopt;
    }

    uint256 GetBestBlock() const final { return m_best_block; }

    void BatchWrite(CoinsViewCacheCursor& cursor, const uint256& block_hash) final
    {
        for (auto it{cursor.Begin()}; it != cursor.End(); it = cursor.NextAndMaybeErase(*it)) {
            if (it->second.IsDirty()) {
                if (it->second.coin.IsSpent()) {
                    m_data.erase(it->first);
                } else {
                    if (cursor.WillErase(*it)) {
                        m_data[it->first] = std::move(it->second.coin);
                    } else {
                        m_data[it->first] = it->second.coin;
                    }
                }
            } else {
                /* For non-dirty entries being written, compare them with what we have. */
                auto it2 = m_data.find(it->first);
                if (it->second.coin.IsSpent()) {
                    assert(it2 == m_data.end());
                } else {
                    assert(it2 != m_data.end());
                    assert(it->second.coin.out == it2->second.out);
                    assert(it->second.coin.fCoinBase == it2->second.fCoinBase);
                    assert(it->second.coin.nHeight == it2->second.nHeight);
                }
            }
        }
        m_best_block = block_hash;
        assert(GetBestBlock() == block_hash);
    }
};

// Hold a non-movable ResetGuard on the heap so StartFetching can remain active
// for the lifetime of a CoinsViewOverlay cache level.
struct OverlayFetchScope
{
    CCoinsViewCache::ResetGuard guard;
    OverlayFetchScope(CoinsViewOverlay& view, const CBlock& block) : guard(view.StartFetching(block)) {}
};

// Reuse a single global thread pool across fuzz iterations. Creating and destroying a pool every
// iteration leaks memory, since iterations can run faster than the OS can tear down the threads.
std::shared_ptr<ThreadPool> g_thread_pool{std::make_shared<ThreadPool>("cache_fuzz")};
Mutex g_thread_pool_mutex;

void StartPoolIfNeeded() EXCLUSIVE_LOCKS_REQUIRED(!g_thread_pool_mutex)
{
    LOCK(g_thread_pool_mutex);
    if (!g_thread_pool->WorkersCount()) g_thread_pool->Start(DEFAULT_PREVOUTFETCH_THREADS);
}

} // namespace

FUZZ_TARGET(coinscache_sim, .init = [] { static auto setup{MakeNoLogFileContext<>()}; }) EXCLUSIVE_LOCKS_REQUIRED(!g_thread_pool_mutex)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    StartPoolIfNeeded();
    /** Precomputed COutPoint and CCoins values. */
    static const PrecomputedData data;

    /** Dummy coinsview instance (base of the hierarchy). */
    CoinsViewBottom bottom;
    /** Real CCoinsViewCache objects. */
    std::vector<std::unique_ptr<CCoinsViewCache>> caches;
    /** Long-lived StartFetching guard (nullptr unless corresponding level is a CoinsViewOverlay). */
    std::unique_ptr<OverlayFetchScope> overlay_fetch_scope;
    /** Whether each corresponding cache entry is a CoinsViewOverlay. */
    std::vector<bool> cache_is_overlay;
    /** Simulated cache data (sim_caches[0] matches bottom, sim_caches[i+1] matches caches[i]). */
    CacheLevel sim_caches[MAX_CACHES + 1];
    /** Simulated local best block hash for each layer; null means inherit from the layer below. */
    uint256 sim_best_blocks[MAX_CACHES + 1]{};
    /** Current height in the simulation. */
    uint32_t current_height = 1U;

    // Initialize bottom simulated cache.
    sim_caches[0].Wipe();

    /** Helper lookup function in the simulated cache stack. */
    auto lookup = [&](uint32_t outpointidx, int sim_idx = -1) -> std::optional<std::pair<coinidx_type, uint32_t>> {
        uint32_t cache_idx = sim_idx == -1 ? caches.size() : sim_idx;
        while (true) {
            const auto& entry = sim_caches[cache_idx].entry[outpointidx];
            if (entry.entrytype == EntryType::UNSPENT) {
                return {{entry.coinidx, entry.height}};
            } else if (entry.entrytype == EntryType::SPENT) {
                return std::nullopt;
            };
            if (cache_idx == 0) break;
            --cache_idx;
        }
        return std::nullopt;
    };

    auto effective_best_block = [&](uint32_t sim_idx) -> uint256 {
        while (sim_idx > 0 && sim_best_blocks[sim_idx].IsNull()) {
            --sim_idx;
        }
        return sim_best_blocks[sim_idx];
    };

    auto assert_best_block = [&](uint32_t sim_idx) {
        assert(sim_idx > 0);
        assert(sim_idx <= caches.size());
        const auto expected{effective_best_block(sim_idx)};
        const auto real{caches[sim_idx - 1]->GetBestBlock()};
        assert(real == expected);
        // GetBestBlock caches inherited values in every null cache layer it descends through.
        while (sim_idx > 0 && sim_best_blocks[sim_idx].IsNull()) {
            sim_best_blocks[sim_idx] = expected;
            --sim_idx;
        }
    };

    /** Flush changes in top cache to the one below. */
    auto flush = [&]() {
        assert(caches.size() >= 1);
        auto& cache = sim_caches[caches.size()];
        auto& prev_cache = sim_caches[caches.size() - 1];
        for (uint32_t outpointidx = 0; outpointidx < NUM_OUTPOINTS; ++outpointidx) {
            if (cache.entry[outpointidx].entrytype != EntryType::NONE) {
                prev_cache.entry[outpointidx] = cache.entry[outpointidx];
                cache.entry[outpointidx].entrytype = EntryType::NONE;
            }
        }
        sim_best_blocks[caches.size() - 1] = sim_best_blocks[caches.size()];
    };

    auto assert_cache_clean = [](const CCoinsViewCache& cache) {
        assert(cache.GetDirtyCount() == 0);
        cache.SanityCheck();
    };

    auto assert_cache_empty = [&](const CCoinsViewCache& cache) {
        assert(cache.GetCacheSize() == 0);
        assert_cache_clean(cache);
    };

    auto assert_cache_stack_sane = [&]() {
        assert(cache_is_overlay.size() == caches.size());
        for (const auto& cache : caches) {
            assert(cache->GetDirtyCount() <= cache->GetCacheSize());
            cache->SanityCheck();
        }
    };

    struct CacheStats {
        size_t cache_size;
        size_t dirty_count;
        size_t memory_usage;
    };

    auto get_cache_stats = [&](size_t cache_count) {
        assert(cache_count <= caches.size());
        std::vector<CacheStats> stats;
        stats.reserve(cache_count);
        for (size_t idx{0}; idx < cache_count; ++idx) {
            const auto& cache{caches[idx]};
            stats.push_back({
                cache->GetCacheSize(),
                cache->GetDirtyCount(),
                cache->DynamicMemoryUsage(),
            });
        }
        return stats;
    };

    auto get_all_cache_stats = [&]() {
        return get_cache_stats(caches.size());
    };

    auto get_dirty_counts = [&]() {
        std::vector<size_t> dirty_counts;
        dirty_counts.reserve(caches.size());
        for (const auto& cache : caches) {
            dirty_counts.push_back(cache->GetDirtyCount());
        }
        return dirty_counts;
    };

    auto get_cache_sizes = [&]() {
        std::vector<size_t> cache_sizes;
        cache_sizes.reserve(caches.size());
        for (const auto& cache : caches) {
            cache_sizes.push_back(cache->GetCacheSize());
        }
        return cache_sizes;
    };

    auto assert_cache_stats = [&](const std::vector<CacheStats>& expected) {
        assert(expected.size() <= caches.size());
        for (size_t idx{0}; idx < expected.size(); ++idx) {
            assert(caches[idx]->GetCacheSize() == expected[idx].cache_size);
            assert(caches[idx]->GetDirtyCount() == expected[idx].dirty_count);
            assert(caches[idx]->DynamicMemoryUsage() == expected[idx].memory_usage);
        }
    };

    auto assert_dirty_counts = [&](const std::vector<size_t>& expected) {
        assert(expected.size() == caches.size());
        for (size_t idx{0}; idx < expected.size(); ++idx) {
            assert(caches[idx]->GetDirtyCount() == expected[idx]);
        }
    };

    auto assert_cache_sizes = [&](const std::vector<size_t>& expected) {
        assert(expected.size() == caches.size());
        for (size_t idx{0}; idx < expected.size(); ++idx) {
            assert(caches[idx]->GetCacheSize() == expected[idx]);
        }
    };

    auto assert_best_block_preserves_cache_stats = [&](uint32_t sim_idx) {
        const auto cache_stats{get_cache_stats(sim_idx)};
        assert_best_block(sim_idx);
        assert_cache_stats(cache_stats);
    };

    auto get_parent_cache_stats_if_top_overlay = [&]() -> std::optional<std::vector<CacheStats>> {
        assert(cache_is_overlay.size() == caches.size());
        if (!cache_is_overlay.empty() && cache_is_overlay.back()) {
            return get_cache_stats(caches.size() - 1);
        }
        return std::nullopt;
    };

    auto assert_cache_stats_if_present = [&](const std::optional<std::vector<CacheStats>>& expected) {
        if (expected) assert_cache_stats(*expected);
    };

    auto assert_spent_public = [&](uint32_t outpointidx) {
        const auto& outpoint{data.outpoints[outpointidx]};
        assert(!caches.back()->HaveCoin(outpoint));
        assert(!caches.back()->GetCoin(outpoint));
        assert(caches.back()->AccessCoin(outpoint).IsSpent());
    };

    auto coins_equal = [](const Coin& a, const Coin& b) {
        if (a.IsSpent() || b.IsSpent()) return a.IsSpent() && b.IsSpent();
        return a.out == b.out && a.fCoinBase == b.fCoinBase && a.nHeight == b.nHeight;
    };

    auto assert_coin_matches_sim = [&](const std::optional<Coin>& realcoin, std::optional<std::pair<coinidx_type, uint32_t>> sim) {
        if (!sim.has_value()) {
            assert(!realcoin);
        } else {
            assert(realcoin && !realcoin->IsSpent());
            const auto& simcoin = data.coins[sim->first];
            assert(realcoin->out == simcoin.out);
            assert(realcoin->fCoinBase == simcoin.fCoinBase);
            assert(realcoin->nHeight == sim->second);
        }
    };

    auto assert_cache_peek_matches_sim = [&](unsigned sim_idx) {
        assert(sim_idx > 0);
        assert(sim_idx <= caches.size());
        const auto cache_stats{get_cache_stats(sim_idx)};
        const auto& cache{*caches[sim_idx - 1]};
        for (uint32_t outpointidx = 0; outpointidx < NUM_OUTPOINTS; ++outpointidx) {
            assert_coin_matches_sim(cache.PeekCoin(data.outpoints[outpointidx]), lookup(outpointidx, sim_idx));
        }
        assert_cache_stats(cache_stats);
    };

    auto assert_stack_peek_matches_sim = [&]() {
        for (unsigned sim_idx = 1; sim_idx <= caches.size(); ++sim_idx) {
            assert_cache_peek_matches_sim(sim_idx);
        }
    };

    auto assert_bottom_matches_sim = [&]() {
        assert(bottom.GetBestBlock() == sim_best_blocks[0]);
        for (uint32_t outpointidx = 0; outpointidx < NUM_OUTPOINTS; ++outpointidx) {
            assert_coin_matches_sim(bottom.GetCoin(data.outpoints[outpointidx]), lookup(outpointidx, 0));
        }
    };

    // Main simulation loop: read commands from the fuzzer input, and apply them
    // to both the real cache stack and the simulation.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    LIMITED_WHILE(provider.remaining_bytes(), 10000) {
        // Every operation (except "Change height") moves current height forward,
        // so it functions as a kind of epoch, making ~all UTXOs unique.
        ++current_height;
        // Make sure there is always at least one CCoinsViewCache.
        if (caches.empty()) {
            caches.emplace_back(new CCoinsViewCache(&bottom, /*deterministic=*/true));
            cache_is_overlay.push_back(false);
            sim_caches[caches.size()].Wipe();
            sim_best_blocks[caches.size()] = uint256::ZERO;
        }

        // Execute command.
        CallOneOf(
            provider,

            [&]() { // PeekCoin/GetCoin
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                // Look up in simulation data.
                auto sim = lookup(outpointidx);
                // Look up in real caches.
                const bool use_peek{provider.ConsumeBool()};
                const auto cache_stats{use_peek ? get_all_cache_stats() : std::vector<CacheStats>{}};
                const auto dirty_counts{get_dirty_counts()};
                const auto parent_cache_stats{use_peek ? std::optional<std::vector<CacheStats>>{} : get_parent_cache_stats_if_top_overlay()};
                auto realcoin = use_peek ?
                    caches.back()->PeekCoin(data.outpoints[outpointidx]) :
                    caches.back()->GetCoin(data.outpoints[outpointidx]);
                if (use_peek) assert_cache_stats(cache_stats);
                assert_dirty_counts(dirty_counts);
                assert_cache_stats_if_present(parent_cache_stats);
                // Compare results.
                if (!sim.has_value()) {
                    assert(!realcoin);
                } else {
                    assert(realcoin && !realcoin->IsSpent());
                    const auto& simcoin = data.coins[sim->first];
                    assert(realcoin->out == simcoin.out);
                    assert(realcoin->fCoinBase == simcoin.fCoinBase);
                    assert(realcoin->nHeight == sim->second);
                }
            },

            [&]() { // HaveCoin
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                // Look up in simulation data.
                auto sim = lookup(outpointidx);
                // Look up in real caches.
                const auto dirty_counts{get_dirty_counts()};
                const auto parent_cache_stats{get_parent_cache_stats_if_top_overlay()};
                auto real = caches.back()->HaveCoin(data.outpoints[outpointidx]);
                assert_dirty_counts(dirty_counts);
                assert_cache_stats_if_present(parent_cache_stats);
                // Compare results.
                assert(sim.has_value() == real);
            },

            [&]() { // HaveInputs
                CMutableTransaction tx;
                bool expected{true};
                const bool is_coinbase{provider.ConsumeBool()};
                if (is_coinbase) {
                    tx.vin.emplace_back(COutPoint{});
                } else {
                    const auto input_count{provider.ConsumeIntegralInRange<uint32_t>(0, 8)};
                    for (uint32_t i{0}; i < input_count; ++i) {
                        const auto outpointidx{provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1)};
                        tx.vin.emplace_back(data.outpoints[outpointidx]);
                        expected &= lookup(outpointidx).has_value();
                    }
                }

                const auto cache_stats{get_all_cache_stats()};
                const auto dirty_counts{get_dirty_counts()};
                const auto parent_cache_stats{get_parent_cache_stats_if_top_overlay()};
                const bool real{caches.back()->HaveInputs(CTransaction{tx})};
                assert(real == expected);
                assert_dirty_counts(dirty_counts);
                assert_cache_stats_if_present(parent_cache_stats);
                if (is_coinbase || tx.vin.empty()) {
                    assert_cache_stats(cache_stats);
                }
            },

            [&]() { // HaveCoinInCache
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                const auto cache_size{caches.back()->GetCacheSize()};
                const auto dirty_count{caches.back()->GetDirtyCount()};
                const auto cache_usage{caches.back()->DynamicMemoryUsage()};
                // This is only a current-cache check. It must not fetch from the backing view.
                const bool has_coin{caches.back()->HaveCoinInCache(data.outpoints[outpointidx])};
                assert(caches.back()->HaveCoinInCache(data.outpoints[outpointidx]) == has_coin);
                assert(caches.back()->GetCacheSize() == cache_size);
                assert(caches.back()->GetDirtyCount() == dirty_count);
                assert(caches.back()->DynamicMemoryUsage() == cache_usage);
            },

            [&]() { // AccessCoin
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                // Look up in simulation data.
                auto sim = lookup(outpointidx);
                // Look up in real caches.
                const auto parent_cache_stats{get_parent_cache_stats_if_top_overlay()};
                const auto& realcoin = caches.back()->AccessCoin(data.outpoints[outpointidx]);
                assert_cache_stats_if_present(parent_cache_stats);
                const Coin realcoin_copy{realcoin};
                const auto cache_stats_after_first{get_all_cache_stats()};
                const auto parent_cache_stats_after_first{get_parent_cache_stats_if_top_overlay()};
                const auto& realcoin_again = caches.back()->AccessCoin(data.outpoints[outpointidx]);
                assert(coins_equal(realcoin_again, realcoin_copy));
                assert_cache_stats(cache_stats_after_first);
                assert_cache_stats_if_present(parent_cache_stats_after_first);
                // Compare results.
                if (!sim.has_value()) {
                    assert(realcoin.IsSpent());
                } else {
                    assert(!realcoin.IsSpent());
                    const auto& simcoin = data.coins[sim->first];
                    assert(simcoin.out == realcoin.out);
                    assert(simcoin.fCoinBase == realcoin.fCoinBase);
                    assert(realcoin.nHeight == sim->second);
                }
            },

            [&]() { // AccessByTxid
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                const auto& txid{data.outpoints[outpointidx].hash};
                std::optional<uint32_t> first_unspent_idx;
                for (uint32_t idx = 0; idx < NUM_OUTPOINTS; ++idx) {
                    if (data.outpoints[idx].hash != txid || !lookup(idx)) continue;
                    if (!first_unspent_idx || data.outpoints[idx].n < data.outpoints[*first_unspent_idx].n) {
                        first_unspent_idx = idx;
                    }
                }
                if (!first_unspent_idx) return;

                const auto dirty_counts{get_dirty_counts()};
                const auto parent_cache_stats{get_parent_cache_stats_if_top_overlay()};
                const auto& realcoin = AccessByTxid(*caches.back(), txid);
                assert_dirty_counts(dirty_counts);
                assert_cache_stats_if_present(parent_cache_stats);
                const Coin realcoin_copy{realcoin};
                const auto cache_sizes_after_first{get_cache_sizes()};
                const auto dirty_counts_after_first{get_dirty_counts()};
                const auto parent_cache_stats_after_first{get_parent_cache_stats_if_top_overlay()};
                const auto& realcoin_again = AccessByTxid(*caches.back(), txid);
                assert(coins_equal(realcoin_again, realcoin_copy));
                assert_cache_sizes(cache_sizes_after_first);
                assert_dirty_counts(dirty_counts_after_first);
                assert_cache_stats_if_present(parent_cache_stats_after_first);
                const auto sim = lookup(*first_unspent_idx);
                assert(sim);
                assert(!realcoin.IsSpent());
                const auto& simcoin = data.coins[sim->first];
                assert(simcoin.out == realcoin.out);
                assert(simcoin.fCoinBase == realcoin.fCoinBase);
                assert(realcoin.nHeight == sim->second);
            },

            [&]() { // AddCoin (only possible_overwrite if necessary)
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                uint32_t coinidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_COINS - 1);
                // Look up in simulation data (to know whether we must set possible_overwrite or not).
                auto sim = lookup(outpointidx);
                // Invoke on real caches.
                Coin coin = data.coins[coinidx];
                coin.nHeight = current_height;
                caches.back()->AddCoin(data.outpoints[outpointidx], std::move(coin), sim.has_value());
                // Apply to simulation data.
                auto& entry = sim_caches[caches.size()].entry[outpointidx];
                entry.entrytype = EntryType::UNSPENT;
                entry.coinidx = coinidx;
                entry.height = current_height;
            },

            [&]() { // AddCoin (always possible_overwrite)
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                uint32_t coinidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_COINS - 1);
                // Invoke on real caches.
                Coin coin = data.coins[coinidx];
                coin.nHeight = current_height;
                caches.back()->AddCoin(data.outpoints[outpointidx], std::move(coin), true);
                // Apply to simulation data.
                auto& entry = sim_caches[caches.size()].entry[outpointidx];
                entry.entrytype = EntryType::UNSPENT;
                entry.coinidx = coinidx;
                entry.height = current_height;
            },

            [&]() { // AddCoin with unspendable output
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                uint32_t coinidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_COINS - 1);
                const auto sim_before{lookup(outpointidx)};
                const auto cache_stats{get_all_cache_stats()};

                Coin coin = data.coins[coinidx];
                coin.nHeight = current_height;
                coin.out.scriptPubKey = CScript{} << OP_RETURN;
                assert(coin.out.scriptPubKey.IsUnspendable());
                caches.back()->AddCoin(data.outpoints[outpointidx], std::move(coin), provider.ConsumeBool());

                assert(lookup(outpointidx) == sim_before);
                assert_cache_stats(cache_stats);
            },

            [&]() { // SpendCoin (moveto = nullptr)
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                // Look up in simulation data (to compare with the returned bool).
                auto sim = lookup(outpointidx);
                const auto failed_spend_cache_sizes{
                    sim ? std::optional<std::vector<size_t>>{} : std::optional{get_cache_sizes()}};
                const auto failed_spend_dirty_counts{
                    sim ? std::optional<std::vector<size_t>>{} : std::optional{get_dirty_counts()}};
                // Invoke on real caches.
                const auto parent_cache_stats{get_parent_cache_stats_if_top_overlay()};
                const bool real = caches.back()->SpendCoin(data.outpoints[outpointidx], nullptr);
                if (failed_spend_cache_sizes) assert_cache_sizes(*failed_spend_cache_sizes);
                if (failed_spend_dirty_counts) assert_dirty_counts(*failed_spend_dirty_counts);
                assert_cache_stats_if_present(parent_cache_stats);
                // Apply to simulation data.
                sim_caches[caches.size()].entry[outpointidx].entrytype = EntryType::SPENT;
                // Compare return value with whether there was an unspent coin to delete.
                assert(real == sim.has_value());
                assert_spent_public(outpointidx);
            },

            [&]() { // SpendCoin (with moveto)
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                // Look up in simulation data (to compare the returned *moveto with).
                auto sim = lookup(outpointidx);
                const auto failed_spend_cache_sizes{
                    sim ? std::optional<std::vector<size_t>>{} : std::optional{get_cache_sizes()}};
                const auto failed_spend_dirty_counts{
                    sim ? std::optional<std::vector<size_t>>{} : std::optional{get_dirty_counts()}};
                // Invoke on real caches.
                Coin realcoin = data.coins[provider.ConsumeIntegralInRange<uint32_t>(0, NUM_COINS - 1)];
                realcoin.nHeight = current_height;
                const Coin realcoin_before{realcoin};
                const auto parent_cache_stats{get_parent_cache_stats_if_top_overlay()};
                const bool real = caches.back()->SpendCoin(data.outpoints[outpointidx], &realcoin);
                if (failed_spend_cache_sizes) assert_cache_sizes(*failed_spend_cache_sizes);
                if (failed_spend_dirty_counts) assert_dirty_counts(*failed_spend_dirty_counts);
                assert_cache_stats_if_present(parent_cache_stats);
                // Apply to simulation data.
                sim_caches[caches.size()].entry[outpointidx].entrytype = EntryType::SPENT;
                // Compare return value with whether there was an unspent coin to delete.
                assert(real == sim.has_value());
                // Compare *moveto with the value expected based on simulation data.
                if (!sim.has_value()) {
                    assert(coins_equal(realcoin, realcoin_before));
                } else {
                    assert(!realcoin.IsSpent());
                    const auto& simcoin = data.coins[sim->first];
                    assert(simcoin.out == realcoin.out);
                    assert(simcoin.fCoinBase == realcoin.fCoinBase);
                    assert(realcoin.nHeight == sim->second);
                }
                assert_spent_public(outpointidx);
            },

            [&]() { // Uncache
                uint32_t outpointidx = provider.ConsumeIntegralInRange<uint32_t>(0, NUM_OUTPOINTS - 1);
                auto sim = lookup(outpointidx);
                const auto parent_cache_stats{get_cache_stats(caches.size() - 1)};
                // Apply to real caches (there is no equivalent in our simulation).
                caches.back()->Uncache(data.outpoints[outpointidx]);
                assert_cache_stats(parent_cache_stats);
                auto realcoin = caches.back()->PeekCoin(data.outpoints[outpointidx]);
                if (!sim.has_value()) {
                    assert(!realcoin);
                } else {
                    assert(realcoin && !realcoin->IsSpent());
                    const auto& simcoin = data.coins[sim->first];
                    assert(realcoin->out == simcoin.out);
                    assert(realcoin->fCoinBase == simcoin.fCoinBase);
                    assert(realcoin->nHeight == sim->second);
                }
            },

            [&]() { // Add a cache level (if not already at the max).
                if (caches.size() != MAX_CACHES) {
                    if (overlay_fetch_scope) {
                        overlay_fetch_scope.reset();
                        sim_caches[caches.size()].Wipe();
                    }
                    // Apply to real caches.
                    if (provider.ConsumeBool()) {
                        caches.emplace_back(new CCoinsViewCache(&*caches.back(), /*deterministic=*/true));
                        cache_is_overlay.push_back(false);
                    } else {
                        caches.emplace_back(new CoinsViewOverlay(&*caches.back(), g_thread_pool, /*deterministic=*/true));
                        cache_is_overlay.push_back(true);
                        auto& overlay{static_cast<CoinsViewOverlay&>(*caches.back())};
                        overlay_fetch_scope = std::make_unique<OverlayFetchScope>(overlay, data.block);
                    }
                    // Apply to simulation data.
                    sim_caches[caches.size()].Wipe();
                    sim_best_blocks[caches.size()] = uint256::ZERO;
                }
            },

            [&]() { // Remove a cache level.
                // Apply to real caches (this reduces caches.size(), implicitly doing the same on the simulation data).
                caches.back()->SanityCheck();
                const auto parent_cache_stats{get_cache_stats(caches.size() - 1)};
                const auto removed_idx{caches.size()};
                overlay_fetch_scope.reset();
                caches.pop_back();
                cache_is_overlay.pop_back();
                assert_cache_stats(parent_cache_stats);
                sim_best_blocks[removed_idx] = uint256::ZERO;
            },

            [&]() { // Flush.
                // CoinsViewOverlay::Flush() must have all inputs consumed before being called
                if (auto* overlay{dynamic_cast<CoinsViewOverlay*>(caches.back().get())};
                    overlay && !overlay->AllInputsConsumed()) {
                    return;
                }
                // Apply to simulation data.
                flush();
                // Apply to real caches.
                caches.back()->Flush(/*reallocate_cache=*/provider.ConsumeBool());
                assert_cache_empty(*caches.back());
            },

            [&]() { // Sync.
                if (overlay_fetch_scope) return; // CoinsViewOverlay::Sync() is never called in production
                size_t expected_cached_unspent{0};
                for (const auto& entry : sim_caches[caches.size()].entry) {
                    expected_cached_unspent += entry.entrytype == EntryType::UNSPENT;
                }
                // Apply to simulation data (note that in our simulation, syncing and flushing is the same thing).
                flush();
                // Apply to real caches.
                caches.back()->Sync();
                assert_cache_clean(*caches.back());
                assert(caches.back()->GetCacheSize() >= expected_cached_unspent);
            },

            [&]() { // Reset.
                sim_caches[caches.size()].Wipe();
                sim_best_blocks[caches.size()] = uint256::ZERO;
                const auto parent_cache_stats{get_cache_stats(caches.size() - 1)};
                // Apply to real caches. Optionally start fetching again.
                if (overlay_fetch_scope && provider.ConsumeBool()) {
                    overlay_fetch_scope.reset();
                    auto& overlay{static_cast<CoinsViewOverlay&>(*caches.back())};
                    overlay_fetch_scope = std::make_unique<OverlayFetchScope>(overlay, data.block);
                } else {
                    (void)caches.back()->CreateResetGuard();
                }
                assert_cache_stats(parent_cache_stats);
                assert_cache_empty(*caches.back());
            },

            [&]() { // GetBestBlock
                assert_best_block_preserves_cache_stats(caches.size());
            },

            [&]() { // SetBestBlock
                const auto block_hash{provider.ConsumeBool() ?
                    uint256::ZERO :
                    data.block_hashes[provider.ConsumeIntegralInRange<uint32_t>(0, NUM_BLOCK_HASHES - 1)]};
                const auto cache_stats{get_all_cache_stats()};
                caches.back()->SetBestBlock(block_hash);
                assert_cache_stats(cache_stats);
                sim_best_blocks[caches.size()] = block_hash;
            },

            [&]() { // GetCacheSize
                (void)caches.back()->GetCacheSize();
            },

            [&]() { // DynamicMemoryUsage
                (void)caches.back()->DynamicMemoryUsage();
            },

            [&]() { // Change height
                current_height = provider.ConsumeIntegralInRange<uint32_t>(1, current_height - 1);
            }
        );

        assert_cache_stack_sane();
    }

    // Sanity check all the remaining caches
    assert_cache_stack_sane();
    assert_stack_peek_matches_sim();

    // Keep this bottom-to-top comparison read-only while overlay workers may still access lower caches.
    const auto cache_stats{get_all_cache_stats()};
    for (unsigned sim_idx = 1; sim_idx <= caches.size(); ++sim_idx) {
        auto& cache = *caches[sim_idx - 1];
        size_t cache_size = 0;

        for (uint32_t outpointidx = 0; outpointidx < NUM_OUTPOINTS; ++outpointidx) {
            cache_size += cache.HaveCoinInCache(data.outpoints[outpointidx]);
            const auto real{cache.PeekCoin(data.outpoints[outpointidx])};
            auto sim = lookup(outpointidx, sim_idx);
            if (!sim.has_value()) {
                assert(!real);
            } else {
                assert(!real->IsSpent());
                assert(real->out == data.coins[sim->first].out);
                assert(real->fCoinBase == data.coins[sim->first].fCoinBase);
                assert(real->nHeight == sim->second);
            }
        }

        // HaveCoinInCache ignores spent coins, so GetCacheSize() may exceed it.
        assert(cache.GetCacheSize() >= cache_size);
        assert_best_block_preserves_cache_stats(sim_idx);
    }
    assert_cache_stats(cache_stats);

    // Compare the bottom coinsview (not a CCoinsViewCache) with sim_cache[0].
    assert_bottom_matches_sim();
}
