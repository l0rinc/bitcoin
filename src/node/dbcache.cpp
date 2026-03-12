// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <common/system_ram.h>
#include <node/dbcache.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace node {
namespace {
size_t GetAutoDbCache(std::optional<size_t> total_ram, bool ibd) noexcept
{
    if (!total_ram) total_ram.emplace(GetTotalRam());
    const size_t usable{*total_ram > RESERVED_RAM ? *total_ram - RESERVED_RAM : 0};
    return std::clamp(usable / (ibd ? 4 : 20), MIN_DEFAULT_DBCACHE, std::min(MAX_DEFAULT_DBCACHE, MAX_DBCACHE_BYTES));
}
} // namespace

size_t GetTotalRam() noexcept
{
    if (const auto mock_total_ram_mib{gArgs.GetIntArg("-mocktotalram")};
        mock_total_ram_mib && *mock_total_ram_mib > 0) {
        const uint64_t mib{static_cast<uint64_t>(std::max<int64_t>(*mock_total_ram_mib, 0))};
        constexpr uint64_t max_size_t{std::numeric_limits<size_t>::max()};
        return mib > (max_size_t >> 20) ? max_size_t : mib << 20;
    }
    return TryGetTotalRam().value_or(FALLBACK_RAM_BYTES);
}

size_t GetDefaultDBCache(std::optional<size_t> total_ram, bool ibd) noexcept
{
    return GetAutoDbCache(total_ram, ibd);
}

bool ShouldWarnOversizedDbCache(size_t dbcache, size_t total_ram) noexcept
{
    return (total_ram < FALLBACK_RAM_BYTES) ? dbcache > GetDefaultDBCache(total_ram)
                                            : dbcache > (total_ram / 100) * 75;
}
} // namespace node
