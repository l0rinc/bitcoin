// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <attributes.h>
#include <coins.h>
#include <logging.h>
#include <primitives/transaction_identifier.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/hasher.h>
#include <util/threadnames.h>

#include <atomic>
#include <barrier>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <unordered_set>
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
    std::atomic_size_t m_input_head{0};

    //! The inputs of the block which is being fetched.
    struct Input {
        //! Workers update this after setting the coin. The main thread waits on this until it is not false.
        std::atomic_bool ready{false};
        //! The outpoint to fetch;
        const COutPoint& outpoint;
        //! The coin that workers will fetch and main thread will insert into cache.
        Coin coin{};

        Input(Input&& other) noexcept : outpoint{other.outpoint} {} // Only moved in setup for reallocation.
        explicit Input(const COutPoint& o LIFETIMEBOUND) noexcept : outpoint{o} {}
    };
    std::vector<Input> m_inputs{};

    /**
     * The set of txids of all txs in the block being fetched.
     * Used to filter out inputs that are created and spent in the same block,
     * since they will not be in the db or the cache.
     */
    std::unordered_set<Txid, SaltedTxidHasher> m_txids{};

    //! DB coins view to fetch from.
    const CCoinsView* m_db{nullptr};
    //! The cache to fetch from.
    const CCoinsViewCache* m_cache{nullptr};

    std::vector<std::thread> m_worker_threads{};
    std::barrier<> m_barrier;
    bool m_request_stop{false};

    void WorkLoop() noexcept
    {
        while (true) {
            m_barrier.arrive_and_wait();
            if (m_request_stop) [[unlikely]] return;

            while (true) {
                const size_t i{m_input_head.fetch_add(1, std::memory_order_relaxed)};
                if (i >= m_inputs.size()) [[unlikely]] break;
                auto& input{m_inputs[i]};
                auto coin{m_cache->GetPossiblySpentCoinFromCache(input.outpoint)};
                if (!coin && !m_txids.contains(input.outpoint.hash)) {
                    try {
                        coin = m_db->GetCoin(input.outpoint);
                    } catch (const std::runtime_error& e) {
                        LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "InputFetcher failed to fetch input: %s.\n", e.what());
                    }
                }
                if (coin && !coin->IsSpent()) input.coin = std::move(*coin);
                // We need release here, so setting coin above happens before the main thread acquires.
                input.ready.store(true, std::memory_order_release);
                input.ready.notify_one();
            }

            m_barrier.arrive_and_wait();
        }
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
        for (size_t i{1}; i < block.vtx.size(); ++i) {
            const auto& tx{block.vtx[i]};
            outputs_count += tx->vout.size();
            m_txids.emplace(tx->GetHash());
            for (const auto& input : tx->vin) m_inputs.emplace_back(input.prevout);
        }

        // Setup shared pointers and start workers.
        m_db = &db;
        m_cache = &cache;
        m_input_head.store(0, std::memory_order_relaxed);
        m_barrier.arrive_and_wait();

        // Insert fetched coins into the temp_cache as they are set to ready.
        temp_cache.Reserve(temp_cache.GetCacheSize() + m_inputs.size() + outputs_count);
        for (auto& input : m_inputs) {
            input.ready.wait(false, std::memory_order_acquire);
            if (input.coin.IsSpent()) continue;
            temp_cache.EmplaceCoinInternalDANGER(COutPoint{input.outpoint}, std::move(input.coin));
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
                WorkLoop();
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
