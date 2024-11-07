// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <attributes.h>
#include <coins.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <tinyformat.h>
#include <util/threadnames.h>

#include <algorithm>
#include <atomic>
#include <barrier>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

/**
 * Helper for fetching inputs from the CoinsDB and CoinsTip and inserting them
 * into the ephemeral cache used in ConnectBlock.
 *
 * It spawns a fixed set of worker threads that fetch Coins for each input
 * in a block. The Coin is moved into the Input struct and then the ready flag
 * is atomically updated to true. The main thread waits on the ready flag
 * until it is true and then inserts it into the temporary cache.
 *
 * Worker threads are synchronized with the main thread using a barrier, which
 * is used at the beginning of fetching to start the workers and at the end to
 * make sure all workers have exited the work loop.
 */
class InputFetcher
{
private:
    //! The latest input being fetched. Workers atomically increment this when fetching.
    std::atomic_uint32_t m_input_head{0};

    //! The inputs of the block which is being fetched.
    struct Input {
        //! Workers set this after setting the coin. The main thread tests this before reading the coin.
        std::atomic_flag ready{};
        //! The outpoint to fetch;
        const COutPoint& outpoint;
        //! The coin that workers will fetch and main thread will insert into cache.
        std::optional<Coin> coin{std::nullopt};

        Input(Input&& other) noexcept : outpoint{other.outpoint} {} // Only moved in setup for reallocation.
        explicit Input(const COutPoint& o LIFETIMEBOUND) noexcept : outpoint{o} {}
    };
    std::vector<Input> m_inputs{};

    /**
     * The set of first 8 bytes of txids of all txs in the block being fetched. This is used to filter out inputs that
     * are created and spent in the same block, since they will not be in the db or the cache.
     * Using only the first 8 bytes is a performance improvement, versus storing the entire 32 bytes. In case of a
     * collision of a txid in a block having the same first 8 bytes of a txid of an input being spent in that block,
     * the input will be skipped by the input fetcher. This input will still be fetched later in ConnectBlock.
     */
    std::vector<uint64_t> m_txids{};

    //! DB coins view to fetch from.
    const CCoinsView* m_db{nullptr};
    //! The cache to fetch from.
    const CCoinsViewCache* m_cache{nullptr};

    std::vector<std::thread> m_worker_threads{};
    std::barrier<> m_barrier;
    bool m_request_stop{false};

    /**
     * Fetches the next input in the queue. Safe to call from any thread once inside the barrier.
     *
     * @return true if there are more inputs in the queue to fetch
     * @return false if there are no more inputs in the queue to fetch
     */
    bool FetchInput() noexcept
    {
        const auto i{m_input_head.fetch_add(1, std::memory_order_relaxed)};
        if (i >= m_inputs.size()) [[unlikely]] return false;
        auto& input{m_inputs[i]};
        if (std::ranges::binary_search(m_txids, input.outpoint.hash.ToUint256().GetUint64(0))) {
            input.ready.test_and_set(std::memory_order_relaxed);
            input.ready.notify_one();
            return true;
        }
        auto coin{m_cache->GetPossiblySpentCoinFromCache(input.outpoint)};
        if (!coin) {
            try {
                coin = m_db->GetCoin(input.outpoint);
            } catch (const std::runtime_error& e) {
                LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "InputFetcher failed to fetch input: %s.", e.what());
            }
        }
        if (coin && !coin->IsSpent()) [[likely]] input.coin.emplace(std::move(*coin));
        // We need release here, so setting coin above happens before the main thread acquires.
        input.ready.test_and_set(std::memory_order_release);
        input.ready.notify_one();
        return true;
    }

public:
    //! Fetch all block inputs from cache or db, and insert into temp_cache.
    void FetchInputs(CCoinsViewCache& temp_cache, const CCoinsViewCache& cache, const CCoinsView& db, const CBlock& block) noexcept
    {
        if (block.vtx.size() <= 1 || m_worker_threads.size() == 0) return;

        // Loop through the inputs of the block and set them in the queue.
        // Construct the set of txids to filter, and count the outputs to reserve for temp_cache.
        m_txids.reserve(block.vtx.size() - 1);
        m_inputs.reserve(2 * block.vtx.size()); // rough guess
        auto outputs_count{block.vtx[0]->vout.size()};
        for (uint32_t i{1}; i < block.vtx.size(); ++i) {
            const auto& tx{block.vtx[i]};
            outputs_count += tx->vout.size();
            m_txids.emplace_back(tx->GetHash().ToUint256().GetUint64(0));
            for (const auto& input : tx->vin) m_inputs.emplace_back(input.prevout);
        }
        std::ranges::sort(m_txids);

        // Setup shared pointers and start workers.
        m_db = &db;
        m_cache = &cache;
        m_input_head.store(0, std::memory_order_relaxed);
        m_barrier.arrive_and_wait();

        // Insert fetched coins into the temp_cache as they are set to ready.
        temp_cache.Reserve(temp_cache.GetCacheSize() + m_inputs.size() + outputs_count);
        for (auto& input : m_inputs) {
            while (!input.ready.test(std::memory_order_acquire)) {
                // Fetch inputs instead of waiting if the next input to insert is not ready yet
                if (!FetchInput()) {
                    // No more work, just wait
                    input.ready.wait(/*old=*/false, std::memory_order_acquire);
                    break;
                }
            }
            if (input.coin) {
                temp_cache.EmplaceCoinInternalDANGER(COutPoint{input.outpoint}, std::move(*input.coin));
            }
        }

        m_barrier.arrive_and_wait();
        // Cleanup after all worker threads have exited the inner loop.
        m_txids.clear();
        m_inputs.clear();
        m_db = nullptr;
        m_cache = nullptr;
    }

    explicit InputFetcher(int32_t worker_thread_count) noexcept : m_barrier{worker_thread_count + 1}
    {
        if (worker_thread_count <= 0) return;
        for (auto n{0}; n < worker_thread_count; ++n) {
            m_worker_threads.emplace_back([this, n] {
                util::ThreadRename(strprintf("inputfetch.%i", n));
                while (true) {
                    m_barrier.arrive_and_wait();
                    if (m_request_stop) [[unlikely]] return;
                    while (FetchInput()) {}
                    m_barrier.arrive_and_wait();
                }
            });
        }
    }

    ~InputFetcher()
    {
        m_request_stop = true;
        m_barrier.arrive_and_drop();
        for (auto& t : m_worker_threads) t.join();
    }
};

#endif // BITCOIN_INPUTFETCHER_H
