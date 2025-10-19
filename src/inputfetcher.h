#ifndef BITCOIN_INPUTFETCHER_H
#define BITCOIN_INPUTFETCHER_H

#include <coins.h>
#include <logging.h>
#include <primitives/transaction.h>
#include <tinyformat.h>
#include <util/hasher.h>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <thread>
#include <unordered_set>
#include <vector>

class InputFetcher
{
    const size_t m_max_thread_count;
    static constexpr size_t PARALLEL_THRESHOLD_MULTIPLIER{2};

public:
    explicit InputFetcher(size_t max_thread_count) : m_max_thread_count{max_thread_count}
    {
        assert(m_max_thread_count > 0);
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
        // std::ranges::sort(missing, [](const auto& a, const auto& b) { return a < b; }); // Sort for disk locality

        try {
            for (const auto& outpoint : missing) {
                if (auto coin{db.GetCoin(outpoint)}) {
                    block_cache.EmplaceCoinInternalDANGER(COutPoint{outpoint}, std::move(*coin));
                } else {
                    throw std::runtime_error("Failed to fetch outpoint from db"); // caught below
                }
            }
        } catch (const std::runtime_error& e) {
            // Database error: will be handled later in validation.
            LogPrintLevel(BCLog::VALIDATION, BCLog::Level::Warning, "InputFetcher failed to fetch input: %s.", e.what());
        }
    }
};

#endif
