// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCK_WITNESS_H
#define BITCOIN_TEST_UTIL_BLOCK_WITNESS_H

#include <primitives/block.h>

#include <algorithm>
#include <cstddef>
#include <span>

inline size_t StripWitness(std::span<std::byte> data)
{
    const auto output{StripBlockWitness(data)};
    std::ranges::copy(output, data.begin());
    return output.size();
}

#endif // BITCOIN_TEST_UTIL_BLOCK_WITNESS_H
