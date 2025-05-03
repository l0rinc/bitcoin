// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SWIFTSYNC_H
#define BITCOIN_SWIFTSYNC_H

#include <vector>
#include <string>
#include <cstdint>

class SwiftSyncHints
{
public:
    struct BlockHints
    {
        std::vector<uint8_t> spent;
        uint16_t count;

        explicit BlockHints(uint16_t c) : spent((c + 7) / 8), count(c) {}
    };

    void Load(const std::string& filename);
    bool IsLoaded() const noexcept { return is_loaded; }
    int GetTerminalBlockHeight() const noexcept { return terminal_height; }
    int GetNextBitPos() const noexcept { return next_bit_pos; }
    void SetCurrentBlockHeight(int block_height);

    bool HasNextBit() const noexcept { return next_bit_pos < current_bitset->count; }
    bool GetNextBit();

private:
    bool is_loaded{false};
    int terminal_height{-1};
    const BlockHints* current_bitset{nullptr};
    int next_bit_pos{0};
    std::vector<BlockHints> block_outputs_bitset;
};

#endif
