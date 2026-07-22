// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/poly1305.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

using boost::multiprecision::cpp_int;

/** Independent RFC 8439 Poly1305 model using arbitrary-precision arithmetic. */
class Poly1305Model
{
    std::vector<std::byte> m_key;
    std::vector<std::byte> m_message;

    static cpp_int LoadLE(std::span<const std::byte> bytes)
    {
        cpp_int result{0};
        for (size_t i = 0; i < bytes.size(); ++i) {
            result += cpp_int{std::to_integer<uint8_t>(bytes[i])} << (8 * i);
        }
        return result;
    }

public:
    explicit Poly1305Model(std::span<const std::byte> key) : m_key(key.begin(), key.end())
    {
        assert(key.size() == Poly1305::KEYLEN);
    }

    Poly1305Model& Update(std::span<const std::byte> message)
    {
        m_message.insert(m_message.end(), message.begin(), message.end());
        return *this;
    }

    void Finalize(std::span<std::byte> tag) const
    {
        assert(tag.size() == Poly1305::TAGLEN);

        std::array<std::byte, Poly1305::KEYLEN> clamped_key;
        std::copy(m_key.begin(), m_key.end(), clamped_key.begin());
        clamped_key[3] &= std::byte{15};
        clamped_key[7] &= std::byte{15};
        clamped_key[11] &= std::byte{15};
        clamped_key[15] &= std::byte{15};
        clamped_key[4] &= std::byte{252};
        clamped_key[8] &= std::byte{252};
        clamped_key[12] &= std::byte{252};

        const cpp_int r = LoadLE(std::span{clamped_key}.first(16));
        const cpp_int s = LoadLE(std::span{clamped_key}.subspan(16));
        const cpp_int modulus = (cpp_int{1} << 130) - 5;
        cpp_int accumulator{0};
        for (size_t offset = 0; offset < m_message.size(); offset += 16) {
            const size_t block_size = std::min<size_t>(16, m_message.size() - offset);
            cpp_int block = LoadLE(std::span{m_message}.subspan(offset, block_size));
            block += cpp_int{1} << (8 * block_size);
            accumulator = ((accumulator + block) * r) % modulus;
        }

        const cpp_int tag_value = (accumulator + s) & ((cpp_int{1} << 128) - 1);
        for (size_t i = 0; i < tag.size(); ++i) {
            const auto value = ((tag_value >> (8 * i)) & 0xFF).convert_to<unsigned>();
            tag[i] = std::byte{static_cast<uint8_t>(value)};
        }
    }
};

void AssertPoly1305(std::span<const std::byte> key, std::span<const std::byte> message,
                    std::span<const size_t> split_boundaries)
{
    Poly1305 full{key};
    Poly1305Model full_model{key};
    full.Update(message);
    full_model.Update(message);

    Poly1305 split{key};
    Poly1305Model split_model{key};
    size_t consumed{0};
    for (const size_t boundary : split_boundaries) {
        assert(consumed <= boundary);
        assert(boundary <= message.size());
        const auto part = message.subspan(consumed, boundary - consumed);
        split.Update(part);
        split_model.Update(part);
        consumed = boundary;
    }
    const auto tail = message.subspan(consumed);
    split.Update(tail);
    split_model.Update(tail);

    std::array<std::byte, Poly1305::TAGLEN> full_tag{}, full_model_tag{};
    std::array<std::byte, Poly1305::TAGLEN> split_tag{}, split_model_tag{};
    full.Finalize(full_tag);
    full_model.Finalize(full_model_tag);
    split.Finalize(split_tag);
    split_model.Finalize(split_model_tag);
    assert(full_tag == full_model_tag);
    assert(split_tag == split_model_tag);
    assert(full_tag == split_tag);
}

void AssertPoly1305Boundaries()
{
    constexpr std::array<std::array<std::byte, Poly1305::KEYLEN>, 4> keys{{
        std::array<std::byte, Poly1305::KEYLEN>{},
        [] {
            std::array<std::byte, Poly1305::KEYLEN> key{};
            key.fill(std::byte{0xFF});
            return key;
        }(),
        [] {
            std::array<std::byte, Poly1305::KEYLEN> key{};
            for (size_t i = 0; i < key.size(); ++i)
                key[i] = std::byte{uint8_t(i)};
            return key;
        }(),
        [] {
            std::array<std::byte, Poly1305::KEYLEN> key{};
            for (size_t i = 0; i < key.size(); ++i)
                key[i] = std::byte{uint8_t(0xA5 ^ i)};
            return key;
        }(),
    }};
    constexpr std::array<size_t, 13> lengths{0, 1, 15, 16, 17, 31, 32, 33, 63, 64, 127, 128, 256};

    for (const auto& key : keys) {
        for (const size_t length : lengths) {
            std::vector<std::byte> message(length);
            for (size_t i = 0; i < message.size(); ++i)
                message[i] = std::byte{uint8_t(i * 17 + length)};
            std::vector<size_t> boundaries{0, std::min<size_t>(1, length), length / 3, length / 2, length};
            std::sort(boundaries.begin(), boundaries.end());
            boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
            AssertPoly1305(key, message, boundaries);
        }
    }
}

} // namespace

FUZZ_TARGET(crypto_poly1305)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    AssertPoly1305Boundaries();
    const auto key = ConsumeFixedLengthByteVector<std::byte>(fuzzed_data_provider, Poly1305::KEYLEN);
    const auto in = ConsumeRandomLengthByteVector<std::byte>(fuzzed_data_provider);
    AssertPoly1305(key, in, {});
}

FUZZ_TARGET(crypto_poly1305_split)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    AssertPoly1305Boundaries();
    // Read key and instantiate two Poly1305 objects with it.
    auto key = provider.ConsumeBytes<std::byte>(Poly1305::KEYLEN);
    key.resize(Poly1305::KEYLEN);
    Poly1305 poly_full{key}, poly_split{key};
    Poly1305Model model_full{key}, model_split{key};

    // Vector that holds all bytes processed so far.
    std::vector<std::byte> total_input;

    // Process input in pieces.
    LIMITED_WHILE (provider.remaining_bytes(), 100) {
        auto in = ConsumeRandomLengthByteVector<std::byte>(provider);
        poly_split.Update(in);
        model_split.Update(in);
        // Update total_input to match what was processed.
        total_input.insert(total_input.end(), in.begin(), in.end());
    }

    // Process entire input at once.
    poly_full.Update(total_input);
    model_full.Update(total_input);

    // Verify both agree.
    std::array<std::byte, Poly1305::TAGLEN> tag_split, tag_full;
    std::array<std::byte, Poly1305::TAGLEN> model_tag_split, model_tag_full;
    poly_split.Finalize(tag_split);
    poly_full.Finalize(tag_full);
    model_split.Finalize(model_tag_split);
    model_full.Finalize(model_tag_full);
    assert(tag_split == model_tag_split);
    assert(tag_full == model_tag_full);
    assert(tag_full == tag_split);
}
