// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_IBD_STATS_H
#define BITCOIN_NODE_IBD_STATS_H

#include <tinyformat.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

namespace node {
/**
 * Process-wide counters for profiling the initial block download pipeline.
 * Experimental instrumentation: every counter is a relaxed atomic so the hot
 * paths only pay for a clock read and an add.
 */
struct IbdStats {
    // Message handler thread: time spent processing/sending vs waiting for work.
    std::atomic<int64_t> msghand_busy_ns{0};
    std::atomic<int64_t> msghand_wait_ns{0};

    // "block" message handling (on the message handler thread).
    std::atomic<int64_t> block_msgs{0};
    std::atomic<int64_t> block_deser_ns{0};    //!< deserialization of the payload (includes txid/wtxid hashing)
    std::atomic<int64_t> block_mutated_ns{0};  //!< IsBlockMutated (merkle and witness roots)
    std::atomic<int64_t> block_accept_ns{0};   //!< CheckBlock + AcceptBlock (includes the block write)
    std::atomic<int64_t> block_write_ns{0};    //!< BlockManager::WriteBlock
    std::atomic<int64_t> block_activate_ns{0}; //!< ActivateBestChain

    // Block connection (any thread that connects blocks).
    std::atomic<int64_t> connect_tip_ns{0};
    std::atomic<int64_t> connect_load_ns{0};
    std::atomic<int64_t> connect_block_ns{0};
    std::atomic<int64_t> connect_undo_ns{0};
    std::atomic<int64_t> blocks_direct{0};    //!< connected from the block provided by the caller
    std::atomic<int64_t> blocks_from_disk{0}; //!< connected from a block read back from disk

    // Block/undo file flushes (fsync).
    std::atomic<int64_t> file_flushes{0};
    std::atomic<int64_t> file_flush_ns{0};

    // Socket thread: bytes received and time spent in transport receive/decode.
    std::atomic<int64_t> net_recv_bytes{0};
    std::atomic<int64_t> net_recv_ns{0};
    std::atomic<int64_t> net_recv_pauses{0}; //!< times a peer's receive was paused by the flood limit

    std::string ToString() const
    {
        const auto s{[](const std::atomic<int64_t>& ns) { return ns.load(std::memory_order_relaxed) / 1e9; }};
        const auto n{[](const std::atomic<int64_t>& c) { return c.load(std::memory_order_relaxed); }};
        const double busy{s(msghand_busy_ns)}, wait{s(msghand_wait_ns)};
        return strprintf("IBD stats: msghand busy=%.1fs wait=%.1fs (%.1f%% busy) | block msgs=%d deser=%.1fs mutated=%.1fs accept=%.1fs (write=%.1fs) activate=%.1fs | connect tip=%.1fs (load=%.1fs connect=%.1fs undo=%.1fs) blocks direct=%d disk=%d | file flushes=%d in %.1fs | net recv=%.2fGB in %.1fs pauses=%d",
                         busy, wait, busy + wait > 0 ? 100.0 * busy / (busy + wait) : 0.0,
                         n(block_msgs), s(block_deser_ns), s(block_mutated_ns), s(block_accept_ns), s(block_write_ns), s(block_activate_ns),
                         s(connect_tip_ns), s(connect_load_ns), s(connect_block_ns), s(connect_undo_ns), n(blocks_direct), n(blocks_from_disk),
                         n(file_flushes), s(file_flush_ns),
                         n(net_recv_bytes) / 1e9, s(net_recv_ns), n(net_recv_pauses));
    }
};

inline IbdStats& GetIbdStats()
{
    static IbdStats stats;
    return stats;
}

inline void AddNs(std::atomic<int64_t>& counter, std::chrono::steady_clock::duration d)
{
    counter.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(d).count(), std::memory_order_relaxed);
}

/** Adds the lifetime of the object to a counter. */
class ScopedNs
{
    std::atomic<int64_t>& m_counter;
    const std::chrono::steady_clock::time_point m_start{std::chrono::steady_clock::now()};

public:
    explicit ScopedNs(std::atomic<int64_t>& counter) : m_counter{counter} {}
    ~ScopedNs() { AddNs(m_counter, std::chrono::steady_clock::now() - m_start); }
};
} // namespace node

#endif // BITCOIN_NODE_IBD_STATS_H
