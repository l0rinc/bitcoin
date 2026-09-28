// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCK_WITNESS_H
#define BITCOIN_TEST_UTIL_BLOCK_WITNESS_H

#include <primitives/block.h>

#include <cstddef>
#include <span>

inline size_t StripWitness(std::span<std::byte> data)
{
    return StripBlockWitness(data);
}

#endif // BITCOIN_TEST_UTIL_BLOCK_WITNESS_H
