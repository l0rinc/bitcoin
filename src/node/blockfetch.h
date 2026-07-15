// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKFETCH_H
#define BITCOIN_NODE_BLOCKFETCH_H

#include <util/expected.h>

#include <memory>
#include <string>

class CBlock;
class CBlockIndex;

namespace node {
struct NodeContext;

/** Read a block from disk, or optionally fetch a pruned active-chain block for local use. */
util::Expected<std::shared_ptr<const CBlock>, std::string> ReadBlockForLocalUse(const NodeContext& node, const CBlockIndex& block_index, bool allow_fetch = true);
} // namespace node

#endif // BITCOIN_NODE_BLOCKFETCH_H
