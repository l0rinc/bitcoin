// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {
void FinalizeAndAssertEqual(Poly1305& a, Poly1305& b)
{
    std::array<std::byte, Poly1305::TAGLEN> tag_a;
    std::array<std::byte, Poly1305::TAGLEN> tag_b;
    a.Finalize(tag_a);
    b.Finalize(tag_b);
    assert(tag_a == tag_b);
}
} // namespace

FUZZ_TARGET(crypto_poly1305)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const auto key = ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Poly1305::KEYLEN);
    const auto in = ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider);

    std::vector<std::byte> tag_out(Poly1305::TAGLEN);
    Poly1305{key}.Update(std::span<const std::byte>{}).Update(in).Update(std::span<const std::byte>{}).Finalize(tag_out);
}

FUZZ_TARGET(crypto_poly1305_split)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    // Read key and instantiate two Poly1305 objects with it.
    auto key = provider.ConsumeBytes<std::byte>(Poly1305::KEYLEN);
    key.resize(Poly1305::KEYLEN);
    Poly1305 poly_full{key}, poly_split{key}, poly_empty_updates{key};

    // Vector that holds all bytes processed so far.
    std::vector<std::byte> total_input;

    // Process input in pieces.
    LIMITED_WHILE (provider.remaining_bytes(), 100) {
        auto in = ConsumeRandomLengthByteVector<std::byte>(provider);
        poly_empty_updates.Update(std::span<const std::byte>{});
        poly_split.Update(in);
        poly_empty_updates.Update(in);
        poly_empty_updates.Update(std::span<const std::byte>{});
        // Update total_input to match what was processed.
        total_input.insert(total_input.end(), in.begin(), in.end());
    }

    // Process entire input at once.
    poly_full.Update(total_input);

    // Verify both agree.
    FinalizeAndAssertEqual(poly_full, poly_split);

    Poly1305 poly_full_with_empty_updates{key};
    poly_full_with_empty_updates.Update(std::span<const std::byte>{}).Update(total_input).Update(std::span<const std::byte>{});
    FinalizeAndAssertEqual(poly_full_with_empty_updates, poly_empty_updates);

    if (total_input.size() <= 256) {
        std::array<std::byte, Poly1305::TAGLEN> tag_full;
        Poly1305{key}.Update(total_input).Finalize(tag_full);
        for (size_t split{0}; split <= total_input.size(); ++split) {
            Poly1305 poly_boundary{key};
            poly_boundary.Update(std::span{total_input}.first(split));
            poly_boundary.Update(std::span<const std::byte>{});
            poly_boundary.Update(std::span{total_input}.subspan(split));
            std::array<std::byte, Poly1305::TAGLEN> tag_boundary;
            poly_boundary.Finalize(tag_boundary);
            assert(tag_boundary == tag_full);
        }
    }
}
