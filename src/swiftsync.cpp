// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <logging.h>
#include <streams.h>
#include <swiftsync.h>
#include <util/fs.h>

#include <string>

void SwiftSyncHints::Load(const std::string& filename)
{
    FILE* file = fsbridge::fopen(fs::u8path(filename), "rb");
    AutoFile hints_file(file);

    // TODO add number of blocks at the beginning of the file to be able to presize block_outputs_bitmap properly

    while (true) {
        uint16_t count;
        hints_file >> count;
        if (count == 0) break; // end-marker

        block_outputs_bitset.emplace_back(count);
        hints_file.read(MakeWritableByteSpan(block_outputs_bitset.back().spent));

        if (!(block_outputs_bitset.size() % 20'000)) {
            LogInfo("SwiftSync hints bitmap: loaded %zu blocks…", block_outputs_bitset.size());
        }
    }

    terminal_height = block_outputs_bitset.size() - 1;
    is_loaded = true;
}

void SwiftSyncHints::SetCurrentBlockHeight(int height)
{
    // TODO: don't crash is hints for certain height are not available
    assert(is_loaded);
    assert(height >= 0 && height <= terminal_height);
    current_bitset = &block_outputs_bitset.at(height);
    next_bit_pos = 0;
}

bool SwiftSyncHints::GetNextBit()
{
    assert(current_bitset);
    if (next_bit_pos >= current_bitset->count) throw std::out_of_range{""};

    const int vector_index{next_bit_pos >> 3};
    const int bit_index{next_bit_pos & 7};
    ++next_bit_pos;
    return (current_bitset->spent.at(vector_index) >> bit_index) & 1U;
}