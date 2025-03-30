// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "caches.h"

#include <node/coins_view_args.h>

#include <common/args.h>
#include <txdb.h>

namespace node {
void ReadCoinsViewArgs(const ArgsManager& args, CoinsViewOptions& options, size_t coins)
{
    if (auto value = args.GetIntArg("-dbbatchsize")) options.batch_write_bytes = static_cast<size_t>(*value);
    else options.batch_write_bytes = GetDefaultDbBatchSize(coins);

    if (auto value = args.GetIntArg("-dbcrashratio")) options.simulate_crash_ratio = *value;
}
} // namespace node
