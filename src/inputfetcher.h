// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <primitives/transaction_identifier.h>
#include <tinyformat.h>
#include <txdb.h>
#include <util/hasher.h>
#include <util/threadnames.h>
#include <util/time.h>

#include <cstdint>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>

/**
 * Input fetcher for fetching inputs from the CoinsDB and inserting
 * into the CoinsTip.
 *
 * The main thread loops through the block and writes all input indexes to a
 * global vector. It then wakes all workers and starts working as well. Each
 * thread assigns itself a range of inputs from the shared vector, and
 * fetches the coins from disk. The outpoint and coin pairs are written to a
 * thread local vector. Once all inputs are fetched, the main thread
 * loops through all thread local vectors and writes the coins to the cache.
 */
class InputFetcher
{
private:
    /**
     * Main thread releases this semaphore for each worker thread.
     * Worker threads acquire this semaphore to start fetching.
     */
    std::counting_semaphore<> m_start_semaphore{0};
    /**
     * Worker threads release this semaphore once they are done fetching.
     * Main thread acquires this semaphore for each worker thread.
     */
    std::counting_semaphore<> m_complete_semaphore{0};

    /**
     * The flattened indexes to each input in the block. The first item in the
     * pair is the index of the tx, and the second is the index of the vin.
     */
    std::vector<std::pair<size_t, size_t>> m_inputs{};

    /**
     * The index of the last input that is being fetched. Workers assign
     * themselves a range of inputs to fetch from m_inputs. They will use
     * this index as the end of their range, and then set this index to the
     * beginning of the range they take for the next worker. Once it gets to
     * zero, all inputs have been assigned and the next worker will wait.
     */
    std::atomic<int64_t> m_input_counter{0};

    /**
     * The vector of vectors of outpoint:coin pairs.
     * Each thread writes the coins it fetches to the vector at its thread
     * index. This way multiple threads can write concurrently to different
     * vectors in a thread safe way. After all threads are finished, the main
     * thread can loop through all vectors and write the coins to the cache.
     */
    std::vector<std::vector<std::pair<COutPoint, Coin>>> m_coins{};

    //! The number of inputs to be assigned at a time
    const size_t m_batch_size;

    /**
     * The set of txids of all txs in the block being fetched.
     * This is used to filter out inputs that are created in the block,
     * since they will not be in the db or the cache.
     */
    std::unordered_set<Txid, SaltedTxidHasher> m_txids{};

    //! DB coins view to fetch from.
    const CCoinsView* m_db{nullptr};
    //! The cache to check if we already have this input.
    const CCoinsViewCache* m_cache{nullptr};
    //! The block whose prevouts we are fetching.
    const CBlock* m_block{nullptr};

    const size_t m_worker_thread_count;
    std::vector<std::thread> m_worker_threads;
    std::atomic<bool> m_request_stop{false};

    void ThreadLoop(int32_t index) noexcept
    {
        while (true) {
            m_start_semaphore.acquire();
            if (m_request_stop.load(std::memory_order_relaxed)) {
                return;
            }
            FetchInputsOnThread(index);
            m_complete_semaphore.release();
        }
    }

    void FetchInputsOnThread(int32_t index) noexcept
    {
        auto& coins{m_coins[index]};
        auto begin{m_input_counter.fetch_sub(m_batch_size, std::memory_order_relaxed)};
        while (begin > 0) {
            try {
                const auto end{std::max(begin - static_cast<int64_t>(m_batch_size), static_cast<int64_t>(0))};
                for (auto i{begin - 1}; i >= end; --i) {
                    const auto [tx_index, vin_index] = m_inputs[i];
                    const auto& outpoint{m_block->vtx[tx_index]->vin[vin_index].prevout};
                    // If an input spends an outpoint from earlier in the
                    // block, it won't be in the cache yet but it also won't be
                    // in the db either.
                    if (m_txids.contains(outpoint.hash)) {
                        continue;
                    }
                    if (m_cache->HaveCoinInCache(outpoint)) {
                        continue;
                    }
                    if (auto coin{m_db->GetCoin(outpoint)}; coin) {
                        coins.emplace_back(outpoint, std::move(*coin));
                    } else {
                        // Missing an input. This block will fail validation.
                        // Skip remaining inputs and continue so main thread
                        // can proceed.
                        m_input_counter.store(0, std::memory_order_relaxed);
                        break;
                    }
                }
            } catch (const std::runtime_error&) {
                // Database error. This will be handled later in validation.
                // Skip remaining inputs and continue so main thread
                // can proceed.
                m_input_counter.store(0, std::memory_order_relaxed);
                break;
            }

            begin = m_input_counter.fetch_sub(m_batch_size, std::memory_order_relaxed);
        }
    }

public:

    explicit InputFetcher(int32_t batch_size, int32_t worker_thread_count) noexcept
        : m_batch_size(batch_size), m_worker_thread_count(worker_thread_count)
    {
        if (worker_thread_count < 1) {
            // Don't do anything if there are no worker threads.
            return;
        }
        m_coins.reserve(worker_thread_count + 1);
        for (auto n{0}; n < worker_thread_count + 1; ++n) {
            m_coins.emplace_back();
        }
        m_worker_threads.reserve(worker_thread_count);
        for (auto n{0}; n < worker_thread_count; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("inputfetch.%i", n));
                ThreadLoop(n);
            });
        }
    }

    // Since this class manages its own resources, which is a thread
    // pool `m_worker_threads`, copy and move operations are not appropriate.
    InputFetcher(const InputFetcher&) = delete;
    InputFetcher& operator=(const InputFetcher&) = delete;
    InputFetcher(InputFetcher&&) = delete;
    InputFetcher& operator=(InputFetcher&&) = delete;

    //! Fetch all block inputs from db, and insert into cache.
    void FetchInputs(CCoinsViewCache& cache,
                     const CCoinsView& db,
                     const CBlock& block) noexcept
    {
        if (m_worker_threads.empty() || block.vtx.size() <= 1) {
            return;
        }

        m_db = &db;
        m_cache = &cache;
        m_block = &block;

        // Loop through the inputs of the block and add them to the queue
        m_txids.reserve(block.vtx.size() - 1);
        for (size_t i{1}; i < block.vtx.size(); ++i) {
            const auto& tx = block.vtx[i];
            for (size_t j{0}; j < tx->vin.size(); ++j) {
                m_inputs.emplace_back(i, j);
            }
            m_txids.emplace(tx->GetHash());
        }

        // Set the input counter to the end of the queue and wake threads
        m_input_counter.store(m_inputs.size(), std::memory_order_relaxed);
        for (size_t i{0}; i < m_worker_thread_count; ++i) {
            m_start_semaphore.release();
        }

        // Have the main thread work too while we wait for other threads
        FetchInputsOnThread(m_worker_thread_count);

        // Wait for all worker threads to complete
        for (size_t i{0}; i < m_worker_thread_count; ++i) {
            m_complete_semaphore.acquire();
        }

        // At this point all threads are done writing to m_coins, so we can
        // safely read from it and insert the fetched coins into the cache.
        for (auto& thread_coins : m_coins) {
            for (auto&& [outpoint, coin] : thread_coins) {
                cache.EmplaceCoinInternalDANGER(std::move(outpoint),
                                                std::move(coin),
                                                /*set_dirty=*/false);
            }
            thread_coins.clear();
        }
        m_txids.clear();
        m_inputs.clear();
    }

    ~InputFetcher()
    {
        m_request_stop.store(true, std::memory_order_relaxed);
        for (size_t i{0}; i < m_worker_thread_count; ++i) {
            m_start_semaphore.release();
        }
        for (std::thread& t : m_worker_threads) {
            t.join();
        }
    }
};

#endif // BITCOIN_INPUTFETCHER_H
