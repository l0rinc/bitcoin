// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <attributes.h>
#include <coins.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <tinyformat.h>
#include <util/threadnames.h>

#include <atomic>
#include <cstdint>
#include <optional>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

static constexpr int32_t WORKER_THREADS{4};

/**
 * CCoinsViewCache subclass that asynchronously fetches all block inputs in parallel during ConnectBlock without
 * mutating the base cache.
 *
 * Only used in ConnectBlock to pass as an ephemeral view that can be reset if the block is invalid.
 * It provides the same interface as CCoinsViewCache, overriding the FetchCoin private method, Reset, and Flush.
 * It adds an additional StartFetching method to provide the block.
 *
 * The cache spawns a fixed set of worker threads on each StartFetching() call to fetch Coins for each input in a block.
 * When FetchCoin() is called, the main thread waits for the corresponding coin to be fetched and returns it.
 */
class CoinsViewCacheAsync : public CCoinsViewCache
{
private:
    //! The latest input not yet being fetched. Workers atomically increment this when fetching.
    mutable std::atomic_uint32_t m_input_head{0};
    //! The latest input not yet accessed by a consumer. Only the main thread increments this.
    mutable uint32_t m_input_tail{0};

    //! The inputs of the block which is being fetched.
    struct InputToFetch {
        //! Workers set this after setting the coin. The main thread tests this before reading the coin.
        std::atomic_flag ready{};
        //! The outpoint of the input to fetch.
        const COutPoint& outpoint;
        //! The coin that workers will fetch and main thread will insert into cache.
        std::optional<Coin> coin{std::nullopt};

        /**
         * We only move when m_inputs reallocates during setup.
         * We never move after work begins, so we don't have to copy other members.
         */
        InputToFetch(InputToFetch&& other) noexcept : outpoint{other.outpoint} {}
        explicit InputToFetch(const COutPoint& o LIFETIMEBOUND) noexcept : outpoint{o} {}
    };
    mutable std::vector<InputToFetch> m_inputs{};

    /**
     * Claim and fetch the next input in the queue. Safe to call from any thread once StartFetching() has queued inputs.
     *
     * @return true if there are more inputs in the queue to fetch
     * @return false if there are no more inputs in the queue to fetch
     */
    bool ProcessInputInBackground() const noexcept
    {
        const auto i{m_input_head.fetch_add(1, std::memory_order_relaxed)};
        if (i >= m_inputs.size()) [[unlikely]] return false;

        auto& input{m_inputs[i]};
        if (auto coin{base->PeekCoin(input.outpoint)}) [[likely]] input.coin.emplace(std::move(*coin));
        // We need release here, so writing coin in the line above happens before the main thread acquires.
        input.ready.test_and_set(std::memory_order_release);
        input.ready.notify_one();
        return true;
    }

    //! Get the index in m_inputs for the given outpoint. Advances m_input_tail if found.
    std::optional<uint32_t> GetInputIndex(const COutPoint& outpoint) const noexcept
    {
        // This assumes ConnectBlock accesses all inputs in the same order as they are added to m_inputs
        // in StartFetching. Some outpoints are not accessed because they are created by the block, so we scan until we
        // come across the requested input. We advance the tail since the input will be cached and not accessed through
        // this method again.
        for (const auto i : std::views::iota(m_input_tail, m_inputs.size())) [[likely]] {
            if (m_inputs[i].outpoint == outpoint) {
                m_input_tail = i + 1;
                return i;
            }
        }
        return std::nullopt;
    }

    CCoinsMap::iterator FetchCoin(const COutPoint& outpoint) const override
    {
        auto [ret, inserted]{cacheCoins.try_emplace(outpoint)};
        if (!inserted) return ret;

        if (const auto i{GetInputIndex(outpoint)}) [[likely]] {
            auto& input{m_inputs[*i]};
            // We need to acquire to match the worker thread's release.
            input.ready.wait(/*old=*/false, std::memory_order_acquire);
            if (input.coin) [[likely]] ret->second.coin = std::move(*input.coin);
        }

        if (ret->second.coin.IsSpent()) [[unlikely]] {
            // We will only get in here for BIP30 checks or a block with missing or spent inputs.
            if (auto coin{base->PeekCoin(outpoint)}) {
                ret->second.coin = std::move(*coin);
            } else {
                cacheCoins.erase(ret);
                return cacheCoins.end();
            }
        }

        cachedCoinsUsage += ret->second.coin.DynamicMemoryUsage();
        return ret;
    }

    std::vector<std::thread> m_worker_threads{};

    //! Stop all worker threads.
    void StopFetching() noexcept
    {
        if (m_worker_threads.empty()) return;
        // Skip fetching the rest of the inputs by moving the head to the end.
        m_input_head.store(m_inputs.size(), std::memory_order_relaxed);
        for (auto& t : m_worker_threads) t.join();
        m_worker_threads.clear();
        m_inputs.clear();
    }

public:
    //! Fetch all block inputs.
    void StartFetching(const CBlock& block) noexcept
    {
        StopFetching();
        m_input_head.store(0, std::memory_order_relaxed);
        m_input_tail = 0;
        m_inputs.clear();

        // Loop through the inputs of the block and set them in the queue.
        for (const auto& tx : block.vtx | std::views::drop(1)) [[likely]] {
            for (const auto& input : tx->vin) [[likely]] m_inputs.emplace_back(input.prevout);
        }
        // Don't start threads if there's nothing to fetch.
        if (m_inputs.empty()) [[unlikely]] return;
        // Spawn per-block worker threads.
        for (const auto n : std::views::iota(0, WORKER_THREADS)) {
            m_worker_threads.emplace_back([this, n] {
                util::ThreadRename(strprintf("inputfetch.%i", n));
                while (ProcessInputInBackground()) [[likely]] {}
            });
        }
    }

    void Reset() noexcept override
    {
        StopFetching();
        m_input_head.store(0, std::memory_order_relaxed);
        m_input_tail = 0;
        CCoinsViewCache::Reset();
    }

    bool Flush(bool will_reuse_cache = true) override
    {
        StopFetching();
        return CCoinsViewCache::Flush(will_reuse_cache);
    }

    explicit CoinsViewCacheAsync(CCoinsView* base_in) noexcept : CCoinsViewCache{base_in} {}
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
