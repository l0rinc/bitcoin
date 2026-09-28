// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <serialize.h>
#include <streams.h>
#include <tinyformat.h>

#include <cstring>
#include <memory>
#include <sstream>
#include <vector>

uint256 CBlockHeader::GetHash() const
{
    return (HashWriter{} << *this).GetHash();
}

std::vector<std::byte> StripBlockWitness(std::span<const std::byte> data)
{
    SpanReader reader{data};
    const auto pos{[&] { return data.size() - reader.size(); }};
    const auto read_size{[&] { return ReadCompactSize(reader); }};
    std::vector<std::byte> output(data.size());
    size_t written{0}, copy_begin{0};
    // Keep [copy_begin, gap_begin), then drop the already-read gap [gap_begin, pos()).
    const auto drop_from{[&](size_t gap_begin) {
        std::memcpy(output.data() + written, data.data() + copy_begin, gap_begin - copy_begin);
        written += gap_begin - copy_begin;
        copy_begin = pos();
    }};

    reader.ignore(GetSerializeSize(CBlockHeader{}));
    const auto tx_count{read_size()};
    for (uint64_t tx{0}; tx < tx_count; ++tx) {
        reader.ignore(sizeof(CTransaction::version));
        const auto marker{pos()};
        auto input_count{read_size()};
        uint8_t flags{0};
        if (input_count == 0) {
            reader >> flags;
            if (flags != 0) {
                drop_from(marker);
                input_count = read_size();
            }
        }
        for (uint64_t input{0}; input < input_count; ++input) {
            reader.ignore(GetSerializeSize(COutPoint{}));
            reader.ignore(read_size()); // scriptSig
            reader.ignore(sizeof(CTxIn::nSequence));
        }
        // With an empty vin and zero flags, the flags byte already encoded an empty vout
        if (input_count != 0 || flags != 0) {
            const auto output_count{read_size()};
            for (uint64_t output{0}; output < output_count; ++output) {
                reader.ignore(sizeof(CTxOut::nValue));
                reader.ignore(read_size()); // scriptPubKey
            }
        }
        if (flags & 1) {
            const auto witness{pos()};
            bool has_witness{false};
            for (uint64_t input{0}; input < input_count; ++input) {
                const auto stack_size{read_size()};
                has_witness |= (stack_size != 0);
                for (uint64_t item{0}; item < stack_size; ++item) reader.ignore(read_size());
            }
            if (!has_witness) throw std::ios_base::failure("Superfluous witness record");
            drop_from(witness);
            flags ^= 1;
        }
        if (flags != 0) throw std::ios_base::failure("Unknown transaction optional data");
        reader.ignore(sizeof(CTransaction::nLockTime));
    }
    drop_from(pos());
    output.resize(written);
    return output;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
