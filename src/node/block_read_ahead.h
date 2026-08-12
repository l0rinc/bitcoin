// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCK_READ_AHEAD_H
#define BITCOIN_NODE_BLOCK_READ_AHEAD_H

#include <chain.h>
#include <flatfile.h>
#include <node/blockstorage.h>
#include <primitives/block.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/thread.h>

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <future>
#include <map>
#include <memory>
#include <ranges>
#include <thread>
#include <utility>
#include <vector>

namespace node {

class BlockReadAhead
{
public:
    explicit BlockReadAhead(const BlockManager& blockman) : m_blockman{blockman} {}
    ~BlockReadAhead() { Stop(); }

    BlockReadAhead(const BlockReadAhead&) = delete;
    BlockReadAhead& operator=(const BlockReadAhead&) = delete;

    void Start(int32_t depth, int32_t threads)
    {
        if (depth <= 0 || threads <= 0 || Enabled()) return;
        m_depth = static_cast<size_t>(depth);
        WITH_LOCK(m_mutex, m_stop = false);
        m_threads.reserve(threads);
        for (int32_t i{0}; i < threads; ++i) {
            m_threads.emplace_back(&util::TraceThread, strprintf("blockread.%d", i), [this] { ReaderThread(); });
        }
    }

    void Stop()
    {
        if (Enabled()) {
            WITH_LOCK(m_mutex, m_stop = true);
            m_cv.notify_all();
            for (auto& thread : m_threads) thread.join();
            m_threads.clear();
        }
        DropBuffered();
    }

    bool Enabled() const { return !m_threads.empty(); }

    void Prime(const std::vector<CBlockIndex*>& to_connect) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        if (!Enabled()) return;
        InFlightMap next;
        bool queued_any{false};
        {
            LOCK(m_mutex);
            for (CBlockIndex* pindex : to_connect | std::views::reverse) {
                if (next.size() >= m_depth) break;
                if (next.contains(pindex)) continue;
                if (auto in_flight{m_inflight.extract(pindex)}) {
                    next.insert(std::move(in_flight));
                    continue;
                }
                if (!(pindex->nStatus & BLOCK_HAVE_DATA)) continue;
                const FlatFilePos pos{pindex->GetBlockPos()};
                if (pos.IsNull()) continue;
                const uint256 hash{pindex->GetBlockHash()};
                ReadTask task{[blockman = &m_blockman, pos, hash]() -> std::shared_ptr<const CBlock> {
                    auto block{std::make_shared<CBlock>()};
                    if (!blockman->ReadBlock(*block, pos, hash)) return nullptr;
                    return block;
                }};
                next.emplace(pindex, task.get_future());
                m_queue.push_back(std::move(task));
                queued_any = true;
            }
        }
        m_inflight = std::move(next);
        if (queued_any) m_cv.notify_all();
    }

    std::shared_ptr<const CBlock> Take(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
    {
        auto it{m_inflight.find(pindex)};
        if (it == m_inflight.end()) return nullptr;
        std::shared_ptr<const CBlock> block{it->second.get()};
        m_inflight.erase(it);
        return block;
    }

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(::cs_main) { DropBuffered(); }

private:
    using ReadTask = std::packaged_task<std::shared_ptr<const CBlock>()>;
    using InFlightMap = std::map<const CBlockIndex*, std::future<std::shared_ptr<const CBlock>>>;

    void DropBuffered()
    {
        m_inflight.clear();
        WITH_LOCK(m_mutex, m_queue.clear());
    }

    void ReaderThread()
    {
        for (;;) {
            ReadTask task;
            {
                WAIT_LOCK(m_mutex, lock);
                m_cv.wait(lock, [&]() EXCLUSIVE_LOCKS_REQUIRED(m_mutex) { return m_stop || !m_queue.empty(); });
                if (m_stop) return;
                task = std::move(m_queue.front());
                m_queue.pop_front();
            }
            task();
        }
    }

    const BlockManager& m_blockman;
    std::vector<std::thread> m_threads;
    size_t m_depth{0};
    InFlightMap m_inflight;

    Mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<ReadTask> m_queue GUARDED_BY(m_mutex);
    bool m_stop GUARDED_BY(m_mutex){false};
};

} // namespace node

#endif // BITCOIN_NODE_BLOCK_READ_AHEAD_H
