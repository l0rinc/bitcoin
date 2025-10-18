#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <threadpool.h>
#include <util/hasher.h>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_set>
#include <vector>

class InputFetcher
{
    ThreadPool m_pool;

public:
    explicit InputFetcher(size_t thread_count) : m_pool{thread_count}
    {
        assert(thread_count > 0);
    }

    void FetchInputs(const CCoinsViewCache& cache, CCoinsViewCache& block_cache, const CCoinsView& db, const CBlock& block) noexcept
    {
        assert(block.vtx.size() >= 1);

        std::unordered_set<Txid, SaltedTxidHasher> block_txids;
        block_txids.reserve(block.vtx.size() - 1);

        // Find missing inputs (not in cache and not created in this block)
        auto input_count{std::accumulate(std::next(block.vtx.cbegin()), block.vtx.cend(), size_t{0}, [](auto sum, const auto& tx) { return sum + tx->vin.size(); } )};
        std::vector<COutPoint> missing;
        missing.reserve(input_count);
        for (uint32_t i{1}; i < block.vtx.size(); ++i) {
            for (const auto& vin : block.vtx[i]->vin) {
                if (!cache.HaveCoinInCache(vin.prevout) && !block_txids.contains(vin.prevout.hash)) {
                    missing.push_back(vin.prevout);
                }
            }
            block_txids.insert(block.vtx[i]->GetHash());
        }
        if (missing.empty()) return;
        std::ranges::sort(missing, [](const auto& a, const auto& b) { return a < b; }); // Sort for disk locality

        const size_t slice{(missing.size() + (m_pool.Size() - 1)) / m_pool.Size()}; // Round up so last thread gets smallest slice
        std::vector<std::vector<std::pair<COutPoint, Coin>>> results(m_pool.Size());
        for (auto& result : results) result.reserve(slice);

        m_pool.Run([&](size_t thread_index) {
            for (size_t i{thread_index * slice}, end{std::min((thread_index + 1) * slice, missing.size())}; i < end; ++i) {
                if (auto coin{db.GetCoin(missing[i])}) {
                    results[thread_index].emplace_back(missing[i], std::move(*coin));
                } else {
                    throw std::runtime_error("Failed to fetch outpoint from db"); // caught by pool, will fail validation
                }
            }
        });

        for (size_t t{0}; t < m_pool.Size(); ++t) {
            for (auto& [outpoint, coin] : results[t]) {
                block_cache.EmplaceCoinInternalDANGER(std::move(outpoint), std::move(coin));
            }
        }
    }
};

#endif
