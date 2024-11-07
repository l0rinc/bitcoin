// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

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
 * in a block. The Coin is moved into the Input struct and then the status is
 * atomically updated to READY. The main thread spin loops on the status field
 * until it is READY and then inserts it into the temporary cache.
 *
 * Worker threads are synchronized with the main thread using a barrier, which
 * is used at the beginning of fetching to start the workers and at the end to
 * make sure all workers have exited the work loop.
 */
class InputFetcher
{
private:

    //! The latest input being fetched. Workers atomically increment this when fetching.
    alignas(64) std::atomic_size_t m_input_head{0};

    //! The inputs of the block which is being fetched.
    struct Input {
        enum class Status : uint8_t {
            WAITING, // The coin has not been fetched yet
            READY, // The coin has been fetched and is ready to be inserted into the cache
            FAILED, // The coin failed to be fetched
            SKIPPED, // The coin is created and spent in the same block so cannot be fetched
        };

        //! Workers update this after setting the coin. The main thread spins on this until it is not WAITING.
        std::atomic<Status> status{Status::WAITING};
        //! The vtx index in the block for this input
        uint32_t vtx{0};
        //! The vin index in the tx for this input
        uint32_t vin{0};
        //! The coin that workers will fetch and main thread will insert into cache.
        Coin coin{};

        Input(Input&& other) noexcept
            : status{other.status.load(std::memory_order_relaxed)},
              vtx{other.vtx}, vin{other.vin}, coin{std::move(other.coin)} {}
        Input(uint32_t i, uint32_t j) noexcept : vtx{i}, vin{j} {};
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
    //! The block to fetch inputs for.
    const CBlock* m_block{nullptr};

    std::vector<std::thread> m_worker_threads;
    std::barrier<> m_barrier;
    bool m_request_stop{false};

    void WorkLoop() noexcept
    {
        while (true) {
            m_barrier.arrive_and_wait();
            if (m_request_stop) [[unlikely]] {
                return;
            }
            while (true) {
                const size_t i{m_input_head.fetch_add(1, std::memory_order_relaxed)};
                if (i >= m_inputs.size()) [[unlikely]] {
                    break;
                }
                auto& input{m_inputs[i]};
                const auto& outpoint{m_block->vtx[input.vtx]->vin[input.vin].prevout};
                // If an input spends an outpoint from earlier in the block,
                // it won't be in the cache yet but it also won't be in the db either.
                if (m_txids.contains(outpoint.hash)) {
                    input.status.store(Input::Status::SKIPPED, std::memory_order_relaxed);
                    continue;
                }
                try {
                    if (auto coin{m_cache->GetPossiblySpentCoinFromCache(outpoint)}) {
                        input.coin = std::move(*coin);
                        if (!input.coin.IsSpent()) [[likely]] { // Coin from cache could be spent
                            // We need release here, so setting coin in the previous line happens before the main thread loads.
                            input.status.store(Input::Status::READY, std::memory_order_release);
                            continue;
                        }
                    } else if (auto coin{m_db->GetCoin(outpoint)}) {
                        input.coin = std::move(*coin); // Coin from db cannot be spent, it does not store spent coins
                        // We need release here, so setting coin in the previous line happens before the main thread loads.
                        input.status.store(Input::Status::READY, std::memory_order_release);
                        continue;
                    }
                } catch (const std::runtime_error& e) {
                    LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "InputFetcher failed to fetch input: %s.\n", e.what());
                }
                // Input missing or spent. This block will fail validation.
                // Skip remaining inputs.
                m_input_head.store(m_inputs.size(), std::memory_order_relaxed);
                input.status.store(Input::Status::FAILED, std::memory_order_relaxed);
            }
            m_barrier.arrive_and_wait();
        }
    }

public:
    explicit InputFetcher(size_t worker_thread_count) noexcept
        : m_barrier{static_cast<int32_t>(worker_thread_count + 1)}
    {
        for (size_t n{0}; n < worker_thread_count; ++n) {
            m_worker_threads.emplace_back([this, n]() {
                util::ThreadRename(strprintf("inputfetch.%i", n));
                WorkLoop();
            });
        }
    }

    //! Fetch all block inputs from cache or db, and insert into temp_cache.
    void FetchInputs(CCoinsViewCache& temp_cache, const CCoinsViewCache& cache, const CCoinsView& db, const CBlock& block) noexcept
    {
        if (block.vtx.size() <= 1 || m_worker_threads.size() == 0) {
            return;
        }

        // Loop through the inputs of the block and set them in the queue.
        // Construct the set of txids to filter, and count the outputs to reserve for temp_cache.
        auto outputs_count{block.vtx[0]->vout.size()};
        for (size_t i{1}; i < block.vtx.size(); ++i) {
            const auto& tx{block.vtx[i]};
            outputs_count += tx->vout.size();
            m_txids.emplace(tx->GetHash());
            for (size_t j{0}; j < tx->vin.size(); ++j) {
                m_inputs.emplace_back(i, j);
            }
        }

        // Setup shared pointers and start workers.
        m_db = &db;
        m_cache = &cache;
        m_block = &block;
        m_input_head.store(0, std::memory_order_relaxed);
        m_barrier.arrive_and_wait();

        temp_cache.Reserve(m_inputs.size() + outputs_count);
        // Insert fetched coins into the temp_cache as they are set to READY.
        for (auto& input : m_inputs) {
            auto status{input.status.load(std::memory_order_acquire)};
            while (status == Input::Status::WAITING) {
                std::this_thread::yield();
                status = input.status.load(std::memory_order_acquire);
            }
            if (status == Input::Status::READY) {
                auto outpoint{block.vtx[input.vtx]->vin[input.vin].prevout};
                temp_cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(input.coin));
            } else if (status == Input::Status::FAILED) [[unlikely]] {
                break;
            }
        }

        m_barrier.arrive_and_wait();
        // Cleanup after all worker threads have exited the inner loop.
        m_txids.clear();
        m_inputs.clear();
        m_db = nullptr;
        m_cache = nullptr;
        m_block = nullptr;
    }

    ~InputFetcher()
    {
        m_request_stop = true;
        m_barrier.arrive_and_wait();
        for (auto& t : m_worker_threads) {
            t.join();
        }
    }
};

#endif // BITCOIN_INPUTFETCHER_H
