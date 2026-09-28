// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_BLOCK_WITNESS_H
#define BITCOIN_TEST_UTIL_BLOCK_WITNESS_H

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>

#include <algorithm>
#include <cstddef>
#include <span>

inline size_t StripWitness(std::span<std::byte> data)
{
    CBlock block;
    SpanReader{data} >> TX_WITH_WITNESS(block);
    DataStream stream;
    stream << TX_NO_WITNESS(block);
    std::ranges::copy(stream, data.begin());
    return stream.size();
}

#endif // BITCOIN_TEST_UTIL_BLOCK_WITNESS_H
