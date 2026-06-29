// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
void AssertNullHeader(const CBlockHeader& block_header)
{
    assert(block_header.IsNull());
    assert(block_header.nVersion == 0);
    assert(block_header.hashPrevBlock.IsNull());
    assert(block_header.hashMerkleRoot.IsNull());
    assert(block_header.nTime == 0);
    assert(block_header.nBits == 0);
    assert(block_header.nNonce == 0);
}

void AssertNullBlock(const CBlock& block)
{
    AssertNullHeader(block);
    assert(block.vtx.empty());
    assert(!block.fChecked);
    assert(!block.m_checked_witness_commitment);
    assert(!block.m_checked_merkle_root);
}
} // namespace

FUZZ_TARGET(block_header)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::optional<CBlockHeader> block_header = ConsumeDeserializable<CBlockHeader>(fuzzed_data_provider);
    if (!block_header) {
        return;
    }
    {
        const uint256 hash = block_header->GetHash();
        constexpr uint256 u256_max{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"};
        assert(hash != u256_max);
        assert(block_header->GetBlockTime() == block_header->nTime);
        assert(block_header->IsNull() == (block_header->nBits == 0));
    }
    {
        CBlockHeader mut_block_header = *block_header;
        mut_block_header.SetNull();
        AssertNullHeader(mut_block_header);
        CBlock block{*block_header};
        assert(block.GetHash() == block_header->GetHash());
        (void)block.ToString();
        block.SetNull();
        AssertNullBlock(block);
        assert(block.GetHash() == mut_block_header.GetHash());
    }
    {
        std::optional<CBlockLocator> block_locator = ConsumeDeserializable<CBlockLocator>(fuzzed_data_provider);
        if (block_locator) {
            (void)block_locator->IsNull();
            block_locator->SetNull();
            assert(block_locator->IsNull());
        }
    }
}
