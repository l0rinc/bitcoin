// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINSVIEWCACHEASYNC_H
#define BITCOIN_COINSVIEWCACHEASYNC_H

#include <attributes.h>
#include <coins.h>
#include <primitives/block.h>
#include <primitives/transaction.h>

#include <ranges>
#include <utility>

/**
 * CCoinsViewCache subclass used by ConnectBlock to prefetch all block inputs without mutating
 * parent caches.
 *
 * This view aims to avoid mutating parent caches while looking up inputs, so an invalid block does not
 * pollute CoinsTip().
 *
 * TODO: Despite the "Async" name, StartFetching() fetches all inputs synchronously in this commit.
 * Later commits parallelize this work.
 */
class CoinsViewCacheAsync : public CCoinsViewCache
{
private:
    CCoinsMap::iterator FetchCoin(const COutPoint& outpoint) const override
    {
        auto [ret, inserted]{cacheCoins.try_emplace(outpoint)};
        if (!inserted) return ret;

        if (auto coin{base->PeekCoin(outpoint)}) {
            ret->second.coin = std::move(*coin);
            cachedCoinsUsage += ret->second.coin.DynamicMemoryUsage();
        } else {
            cacheCoins.erase(ret);
            return cacheCoins.end();
        }
        return ret;
    }

public:
    //! Fetch all block inputs.
    void StartFetching(const CBlock& block) noexcept
    {
        for (const auto& tx : block.vtx | std::views::drop(1)) [[likely]] {
            for (const auto& input : tx->vin) [[likely]] {
                (void)AccessCoin(input.prevout);
            }
        }
    }

    explicit CoinsViewCacheAsync(CCoinsView* base_in) noexcept : CCoinsViewCache{base_in} {}
};

#endif // BITCOIN_COINSVIEWCACHEASYNC_H
