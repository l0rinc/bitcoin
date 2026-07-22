// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {
using Digest = std::array<unsigned char, 32>;
using Bytes = std::vector<unsigned char>;

constexpr size_t SHA256_BLOCK_SIZE{64};

uint32_t RotateRight(uint32_t value, unsigned int bits)
{
    return (value >> bits) | (value << (32 - bits));
}

uint32_t LoadBigEndian(const unsigned char* data)
{
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

void StoreBigEndian(unsigned char* data, uint32_t value)
{
    data[0] = static_cast<unsigned char>(value >> 24);
    data[1] = static_cast<unsigned char>(value >> 16);
    data[2] = static_cast<unsigned char>(value >> 8);
    data[3] = static_cast<unsigned char>(value);
}

Digest ReferenceSHA256(std::span<const unsigned char> input)
{
    static constexpr std::array<uint32_t, 64> round_constants{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};

    std::vector<unsigned char> padded(input.begin(), input.end());
    const uint64_t bit_length{static_cast<uint64_t>(input.size()) * 8};
    padded.push_back(0x80);
    while (padded.size() % SHA256_BLOCK_SIZE != 56) {
        padded.push_back(0);
    }
    for (int shift{56}; shift >= 0; shift -= 8) {
        padded.push_back(static_cast<unsigned char>(bit_length >> shift));
    }

    std::array<uint32_t, 8> state{
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    for (size_t offset{0}; offset < padded.size(); offset += SHA256_BLOCK_SIZE) {
        std::array<uint32_t, 64> schedule{};
        for (size_t i{0}; i < 16; ++i) {
            schedule[i] = LoadBigEndian(padded.data() + offset + i * sizeof(uint32_t));
        }
        for (size_t i{16}; i < schedule.size(); ++i) {
            const uint32_t s0{RotateRight(schedule[i - 15], 7) ^ RotateRight(schedule[i - 15], 18) ^
                              (schedule[i - 15] >> 3)};
            const uint32_t s1{RotateRight(schedule[i - 2], 17) ^ RotateRight(schedule[i - 2], 19) ^
                              (schedule[i - 2] >> 10)};
            schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
        }

        uint32_t a{state[0]};
        uint32_t b{state[1]};
        uint32_t c{state[2]};
        uint32_t d{state[3]};
        uint32_t e{state[4]};
        uint32_t f{state[5]};
        uint32_t g{state[6]};
        uint32_t h{state[7]};
        for (size_t i{0}; i < schedule.size(); ++i) {
            const uint32_t s1{RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25)};
            const uint32_t choice{(e & f) ^ (~e & g)};
            const uint32_t temp1{h + s1 + choice + round_constants[i] + schedule[i]};
            const uint32_t s0{RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22)};
            const uint32_t majority{(a & b) ^ (a & c) ^ (b & c)};
            const uint32_t temp2{s0 + majority};
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    Digest digest{};
    for (size_t i{0}; i < state.size(); ++i) {
        StoreBigEndian(digest.data() + i * sizeof(uint32_t), state[i]);
    }
    return digest;
}

Digest ReferenceHMAC(std::span<const unsigned char> key, std::span<const unsigned char> data)
{
    std::array<unsigned char, SHA256_BLOCK_SIZE> key_block{};
    if (key.size() > SHA256_BLOCK_SIZE) {
        const Digest hashed_key{ReferenceSHA256(key)};
        std::copy(hashed_key.begin(), hashed_key.end(), key_block.begin());
    } else {
        std::copy(key.begin(), key.end(), key_block.begin());
    }

    std::vector<unsigned char> inner(SHA256_BLOCK_SIZE + data.size());
    std::vector<unsigned char> outer(SHA256_BLOCK_SIZE + Digest{}.size());
    for (size_t i{0}; i < SHA256_BLOCK_SIZE; ++i) {
        inner[i] = key_block[i] ^ 0x36;
        outer[i] = key_block[i] ^ 0x5c;
    }
    std::copy(data.begin(), data.end(), inner.begin() + SHA256_BLOCK_SIZE);
    const Digest inner_hash{ReferenceSHA256(inner)};
    std::copy(inner_hash.begin(), inner_hash.end(), outer.begin() + SHA256_BLOCK_SIZE);
    return ReferenceSHA256(outer);
}

std::span<const unsigned char> AsBytes(const std::string& data)
{
    return {reinterpret_cast<const unsigned char*>(data.data()), data.size()};
}

std::span<const unsigned char> AsBytes(const Bytes& data)
{
    return {data.data(), data.size()};
}

std::span<const unsigned char> AsBytes(const Digest& data)
{
    return {data.data(), data.size()};
}

Digest ReferenceHKDF(const Bytes& ikm, const std::string& salt, const std::string& info)
{
    assert(info.size() <= 128);
    const Digest prk{ReferenceHMAC(AsBytes(salt), AsBytes(ikm))};
    std::array<unsigned char, 129> expand_input{};
    std::copy(info.begin(), info.end(), expand_input.begin());
    expand_input[info.size()] = 1;
    return ReferenceHMAC(AsBytes(prk), std::span<const unsigned char>{expand_input}.first(info.size() + 1));
}

Bytes Pattern(size_t size, unsigned char seed)
{
    Bytes result(size);
    for (size_t i{0}; i < result.size(); ++i) {
        result[i] = static_cast<unsigned char>(seed + i * 29);
    }
    return result;
}

std::string PatternString(size_t size, unsigned char seed)
{
    const Bytes bytes{Pattern(size, seed)};
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::string SequenceString(size_t size, unsigned char first)
{
    std::string result(size, '\0');
    for (size_t i{0}; i < size; ++i) {
        result[i] = static_cast<char>(first + i);
    }
    return result;
}

void AssertExpansion(CHKDF_HMAC_SHA256_L32& hkdf, const Bytes& ikm, const std::string& salt,
                     const std::string& info)
{
    const Digest expected{ReferenceHKDF(ikm, salt, info)};
    std::array<unsigned char, 32> actual;
    actual.fill(0xA5);
    hkdf.Expand32(info, actual.data());
    assert(actual == expected);

    std::array<unsigned char, 32> repeated;
    repeated.fill(0x5A);
    hkdf.Expand32(info, repeated.data());
    assert(repeated == actual);
}

void AssertHKDFCase(const Bytes& ikm, const std::string& salt, const std::string& info)
{
    CHKDF_HMAC_SHA256_L32 hkdf(ikm.data(), ikm.size(), salt);
    AssertExpansion(hkdf, ikm, salt, info);
}

void AssertHKDFBoundaries()
{
    const std::array<size_t, 9> key_lengths{{0, 1, 31, 32, 63, 64, 65, 128, 1024}};
    const std::array<size_t, 8> info_lengths{{0, 1, 55, 56, 63, 64, 127, 128}};

    for (size_t i{0}; i < key_lengths.size(); ++i) {
        const Bytes ikm{Pattern(key_lengths[i], static_cast<unsigned char>(i))};
        for (size_t j{0}; j < key_lengths.size(); ++j) {
            const std::string salt{PatternString(key_lengths[j], static_cast<unsigned char>(0x40 + j))};
            CHKDF_HMAC_SHA256_L32 hkdf(ikm.data(), ikm.size(), salt);
            for (size_t k{0}; k < info_lengths.size(); ++k) {
                AssertExpansion(hkdf, ikm, salt, PatternString(info_lengths[k], static_cast<unsigned char>(0x80 + k)));
            }
        }
    }

    const Bytes rfc_ikm(22, 0x0b);
    const std::string rfc_salt{SequenceString(13, 0)};
    const std::string rfc_info{SequenceString(10, 0xf0)};
    const Digest rfc_expected{
        0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a,
        0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a,
        0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
        0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf};
    assert(ReferenceHKDF(rfc_ikm, rfc_salt, rfc_info) == rfc_expected);
    AssertHKDFCase(rfc_ikm, rfc_salt, rfc_info);
}
} // namespace

FUZZ_TARGET(crypto_hkdf_hmac_sha256_l32)
{
    AssertHKDFBoundaries();

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> initial_key_material = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::string salt{fuzzed_data_provider.ConsumeRandomLengthString(1024)};

    CHKDF_HMAC_SHA256_L32 hkdf_hmac_sha256_l32(initial_key_material.data(), initial_key_material.size(), salt);
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::string info{fuzzed_data_provider.ConsumeRandomLengthString(128)};
        AssertExpansion(hkdf_hmac_sha256_l32, initial_key_material, salt, info);
    }
}
