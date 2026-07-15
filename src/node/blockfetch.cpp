// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/blockfetch.h>

#include <chain.h>
#include <net_processing.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <primitives/block.h>
#include <sync.h>
#include <util/check.h>
#include <util/expected.h>
#include <validation.h>

#include <utility>

namespace node {
util::Expected<std::shared_ptr<const CBlock>, std::string> ReadBlockForLocalUse(const NodeContext& node, const CBlockIndex& block_index, bool allow_fetch)
{
    ChainstateManager& chainman{*Assert(node.chainman)};
    const bool have_data{WITH_LOCK(cs_main, return (block_index.nStatus & BLOCK_HAVE_DATA) != 0)};
    if (have_data) {
        auto block{std::make_shared<CBlock>()};
        if (chainman.m_blockman.ReadBlock(*block, block_index)) return std::shared_ptr<const CBlock>{std::move(block)};

        // Pruning may have raced the read. Treat a still-present block as an I/O
        // failure rather than hiding corruption behind a network fetch.
        if (WITH_LOCK(cs_main, return (block_index.nStatus & BLOCK_HAVE_DATA) != 0)) {
            return util::Unexpected{"Block data could not be read from disk"};
        }
    }

    if (!allow_fetch) return util::Unexpected{"Block not found on disk"};
    if (!node.peerman) return util::Unexpected{"Peer manager is not available"};
    return node.peerman->FetchBlockForLocalUse(block_index);
}
} // namespace node
