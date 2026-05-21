// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/database_args.h>

#include <common/args.h>
#include <dbwrapper.h>

namespace node {
void ReadDatabaseArgs(const ArgsManager& args, DBOptions& options)
{
    // Hidden compatibility option for chainstate startup compaction.
    if (auto value = args.GetBoolArg("-forcecompactdb")) options.force_compact = *value;
}
} // namespace node
