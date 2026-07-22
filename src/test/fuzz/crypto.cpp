// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hmac_sha256.h>
#include <crypto/hmac_sha512.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha3.h>
#include <crypto/sha512.h>
#include <hash.h>
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
#include <string_view>
#include <vector>

namespace {
using Bytes = std::vector<unsigned char>;

template <typename Hasher, size_t OUTPUT_SIZE>
std::array<unsigned char, OUTPUT_SIZE> HashChunks(const Bytes& data, bool split)
{
    Hasher hasher;
    if (!split) {
        hasher.Write(data.data(), data.size());
    } else {
        constexpr std::array<size_t, 6> chunk_sizes{{1, 2, 63, 64, 65, 127}};
        size_t offset{0};
        size_t chunk_index{0};
        while (offset < data.size()) {
            const size_t chunk_size{std::min(chunk_sizes[chunk_index % chunk_sizes.size()], data.size() - offset)};
            hasher.Write(data.data() + offset, chunk_size);
            offset += chunk_size;
            ++chunk_index;
        }
    }
    std::array<unsigned char, OUTPUT_SIZE> output{};
    hasher.Finalize(output.data());
    return output;
}

std::array<unsigned char, SHA3_256::OUTPUT_SIZE> SHA3Chunks(const Bytes& data, bool split)
{
    SHA3_256 hasher;
    if (!split) {
        hasher.Write(std::span<const unsigned char>{data});
    } else {
        constexpr std::array<size_t, 6> chunk_sizes{{1, 2, 63, 64, 65, 127}};
        size_t offset{0};
        size_t chunk_index{0};
        while (offset < data.size()) {
            const size_t chunk_size{std::min(chunk_sizes[chunk_index % chunk_sizes.size()], data.size() - offset)};
            hasher.Write(std::span<const unsigned char>{data}.subspan(offset, chunk_size));
            offset += chunk_size;
            ++chunk_index;
        }
    }
    std::array<unsigned char, SHA3_256::OUTPUT_SIZE> output{};
    hasher.Finalize(std::span<unsigned char>{output});
    return output;
}

template <typename Hmac, size_t OUTPUT_SIZE>
std::array<unsigned char, OUTPUT_SIZE> HMACChunks(const Bytes& key, const Bytes& data, bool split)
{
    Hmac hmac{key.data(), key.size()};
    if (!split) {
        hmac.Write(data.data(), data.size());
    } else {
        constexpr std::array<size_t, 6> chunk_sizes{{1, 2, 63, 64, 65, 127}};
        size_t offset{0};
        size_t chunk_index{0};
        while (offset < data.size()) {
            const size_t chunk_size{std::min(chunk_sizes[chunk_index % chunk_sizes.size()], data.size() - offset)};
            hmac.Write(data.data() + offset, chunk_size);
            offset += chunk_size;
            ++chunk_index;
        }
    }
    std::array<unsigned char, OUTPUT_SIZE> output{};
    hmac.Finalize(output.data());
    return output;
}

template <size_t OUTPUT_SIZE>
void AssertDigest(const std::array<unsigned char, OUTPUT_SIZE>& actual, std::string_view expected_hex)
{
    const std::vector<unsigned char> expected{ParseHex(std::string{expected_hex})};
    assert(expected.size() == OUTPUT_SIZE);
    assert(std::equal(actual.begin(), actual.end(), expected.begin()));
}

template <typename Hasher, size_t OUTPUT_SIZE>
void AssertHashVector(const Bytes& data, std::string_view expected_hex)
{
    const auto one_shot{HashChunks<Hasher, OUTPUT_SIZE>(data, false)};
    const auto split{HashChunks<Hasher, OUTPUT_SIZE>(data, true)};
    assert(one_shot == split);
    AssertDigest(one_shot, expected_hex);
}

void AssertSHA3Vector(const Bytes& data, std::string_view expected_hex)
{
    const auto one_shot{SHA3Chunks(data, false)};
    const auto split{SHA3Chunks(data, true)};
    assert(one_shot == split);
    AssertDigest(one_shot, expected_hex);
}

template <typename Hmac, size_t OUTPUT_SIZE>
void AssertHMACVector(const Bytes& key, const Bytes& data, std::string_view expected_hex)
{
    const auto one_shot{HMACChunks<Hmac, OUTPUT_SIZE>(key, data, false)};
    const auto split{HMACChunks<Hmac, OUTPUT_SIZE>(key, data, true)};
    assert(one_shot == split);
    AssertDigest(one_shot, expected_hex);
}

Bytes Pattern(size_t size, unsigned char seed)
{
    Bytes result(size);
    for (size_t i{0}; i < size; ++i) {
        result[i] = static_cast<unsigned char>(seed + i * 29);
    }
    return result;
}

template <typename Hasher, size_t OUTPUT_SIZE>
void AssertHashStreaming(const Bytes& data)
{
    const auto one_shot{HashChunks<Hasher, OUTPUT_SIZE>(data, false)};
    const auto split{HashChunks<Hasher, OUTPUT_SIZE>(data, true)};
    assert(one_shot == split);
}

void AssertSHA3Streaming(const Bytes& data)
{
    assert(SHA3Chunks(data, false) == SHA3Chunks(data, true));
}

template <typename Hmac, size_t OUTPUT_SIZE>
void AssertHMACStreaming(const Bytes& key, const Bytes& data)
{
    const auto one_shot{HMACChunks<Hmac, OUTPUT_SIZE>(key, data, false)};
    const auto split{HMACChunks<Hmac, OUTPUT_SIZE>(key, data, true)};
    assert(one_shot == split);
}

void AssertCompoundHash(const Bytes& data, std::string_view expected_hash256, std::string_view expected_hash160)
{
    const auto expected256{ParseHex(std::string{expected_hash256})};
    const auto expected160{ParseHex(std::string{expected_hash160})};
    assert(expected256.size() == CHash256::OUTPUT_SIZE);
    assert(expected160.size() == CHash160::OUTPUT_SIZE);

    CHash256 hash256_one;
    hash256_one.Write(std::span<const unsigned char>{data});
    std::array<unsigned char, CHash256::OUTPUT_SIZE> output256_one{};
    hash256_one.Finalize(std::span<unsigned char>{output256_one});

    CHash256 hash256_split;
    constexpr std::array<size_t, 6> chunk_sizes{{1, 2, 63, 64, 65, 127}};
    size_t offset{0};
    size_t chunk_index{0};
    while (offset < data.size()) {
        const size_t chunk_size{std::min(chunk_sizes[chunk_index % chunk_sizes.size()], data.size() - offset)};
        hash256_split.Write(std::span<const unsigned char>{data}.subspan(offset, chunk_size));
        offset += chunk_size;
        ++chunk_index;
    }
    std::array<unsigned char, CHash256::OUTPUT_SIZE> output256_split{};
    hash256_split.Finalize(std::span<unsigned char>{output256_split});

    CHash160 hash160_one;
    hash160_one.Write(std::span<const unsigned char>{data});
    std::array<unsigned char, CHash160::OUTPUT_SIZE> output160_one{};
    hash160_one.Finalize(std::span<unsigned char>{output160_one});

    CHash160 hash160_split;
    offset = 0;
    chunk_index = 0;
    while (offset < data.size()) {
        const size_t chunk_size{std::min(chunk_sizes[chunk_index % chunk_sizes.size()], data.size() - offset)};
        hash160_split.Write(std::span<const unsigned char>{data}.subspan(offset, chunk_size));
        offset += chunk_size;
        ++chunk_index;
    }
    std::array<unsigned char, CHash160::OUTPUT_SIZE> output160_split{};
    hash160_split.Finalize(std::span<unsigned char>{output160_split});

    assert(output256_one == output256_split);
    assert(output160_one == output160_split);
    assert(std::equal(output256_one.begin(), output256_one.end(), expected256.begin()));
    assert(std::equal(output160_one.begin(), output160_one.end(), expected160.begin()));

    const uint256 hash256{Hash(data)};
    const uint160 hash160{Hash160(data)};
    assert(std::equal(hash256.begin(), hash256.end(), expected256.begin()));
    assert(std::equal(hash160.begin(), hash160.end(), expected160.begin()));
}

void AssertSipHashVectors()
{
    static constexpr std::array<uint64_t, 17> expected{
        0x726fdb47dd0e0e31, 0x74f839c593dc67fd, 0x0d6c8009d9a94f5a, 0x85676696d7fb7e2d,
        0xcf2794e0277187b7, 0x18765564cd99a68d, 0xcbc9466e58fee3ce, 0xab0200f58b01d137,
        0x93f5f5799a932462, 0x9e0082df0ba9e4b0, 0x7a5dbbc594ddb9f3, 0xf4b32f46226bada7,
        0x751e8fbc860ee5fb, 0x14ea5627c0843d90, 0xf723ca908e7af2ee, 0xa129ca6149be45e5,
        0x3f2acc7f57c29bdb};
    Bytes message(16);
    for (size_t i{0}; i < message.size(); ++i) {
        message[i] = static_cast<unsigned char>(i);
    }
    for (size_t size{0}; size <= message.size(); ++size) {
        CSipHasher one_shot{0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL};
        one_shot.Write(std::span<const unsigned char>{message}.first(size));
        assert(one_shot.Finalize() == expected[size]);

        CSipHasher split{0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL};
        for (size_t i{0}; i < size; ++i) {
            split.Write(std::span<const unsigned char>{message}.subspan(i, 1));
        }
        assert(split.Finalize() == one_shot.Finalize());
    }
}

void AssertCryptoBoundaries()
{
    const Bytes empty;
    const Bytes abc{'a', 'b', 'c'};

    AssertHashVector<CSHA1, CSHA1::OUTPUT_SIZE>(empty, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    AssertHashVector<CSHA1, CSHA1::OUTPUT_SIZE>(abc, "a9993e364706816aba3e25717850c26c9cd0d89d");
    AssertHashVector<CSHA256, CSHA256::OUTPUT_SIZE>(empty, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    AssertHashVector<CSHA256, CSHA256::OUTPUT_SIZE>(abc, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    AssertHashVector<CSHA512, CSHA512::OUTPUT_SIZE>(empty, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
    AssertHashVector<CSHA512, CSHA512::OUTPUT_SIZE>(abc, "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    AssertHashVector<CRIPEMD160, CRIPEMD160::OUTPUT_SIZE>(empty, "9c1185a5c5e9fc54612808977ee8f548b2258d31");
    AssertHashVector<CRIPEMD160, CRIPEMD160::OUTPUT_SIZE>(abc, "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
    AssertSHA3Vector(empty, "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");
    AssertSHA3Vector(abc, "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532");

    const Bytes hmac_key{'J', 'e', 'f', 'e'};
    const Bytes hmac_data{'w', 'h', 'a', 't', ' ', 'd', 'o', ' ', 'y', 'a', ' ', 'w', 'a', 'n', 't', ' ', 'f', 'o', 'r', ' ', 'n', 'o', 't', 'h', 'i', 'n', 'g', '?'};
    AssertHMACVector<CHMAC_SHA256, CHMAC_SHA256::OUTPUT_SIZE>(hmac_key, hmac_data, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    AssertHMACVector<CHMAC_SHA512, CHMAC_SHA512::OUTPUT_SIZE>(hmac_key, hmac_data, "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737");
    AssertCompoundHash(empty, "5df6e0e2761359d30a8275058e299fcc0381534545f55cf43e41983f5d4c9456", "b472a266d0bd89c13706a4132ccfb16f7c3b9fcb");
    AssertCompoundHash(abc, "4f8b42c22dd3729b519ba6f68d2da7cc5b2d606d05daed5ad5128cc03e6c6358", "bb1be98c142444d7a56aa3981c3942a978e4dc33");

    for (const size_t size : std::array<size_t, 6>{{1, 63, 64, 65, 127, 128}}) {
        const Bytes data{Pattern(size, static_cast<unsigned char>(size))};
        AssertHashStreaming<CSHA1, CSHA1::OUTPUT_SIZE>(data);
        AssertHashStreaming<CSHA256, CSHA256::OUTPUT_SIZE>(data);
        AssertHashStreaming<CSHA512, CSHA512::OUTPUT_SIZE>(data);
        AssertHashStreaming<CRIPEMD160, CRIPEMD160::OUTPUT_SIZE>(data);
        AssertSHA3Streaming(data);
        AssertHMACStreaming<CHMAC_SHA256, CHMAC_SHA256::OUTPUT_SIZE>(Pattern(65, 0x11), data);
        AssertHMACStreaming<CHMAC_SHA512, CHMAC_SHA512::OUTPUT_SIZE>(Pattern(129, 0x22), data);
        AssertCompoundHash(data, HexStr(Hash(data)), HexStr(Hash160(data)));
    }
    AssertSipHashVectors();
}
} // namespace

FUZZ_TARGET(crypto)
{
    AssertCryptoBoundaries();

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (data.empty()) {
        auto new_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096);
        auto x = fuzzed_data_provider.ConsumeIntegral<uint8_t>();
        data.resize(new_size, x);
    }

    CHash160 hash160;
    CHash256 hash256;
    CHMAC_SHA256 hmac_sha256{data.data(), data.size()};
    CHMAC_SHA512 hmac_sha512{data.data(), data.size()};
    CRIPEMD160 ripemd160;
    CSHA1 sha1;
    CSHA256 sha256;
    CSHA512 sha512;
    SHA3_256 sha3;
    CSipHasher sip_hasher{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>()};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                if (fuzzed_data_provider.ConsumeBool()) {
                    data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                    if (data.empty()) {
                        auto new_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096);
                        auto x = fuzzed_data_provider.ConsumeIntegral<uint8_t>();
                        data.resize(new_size, x);
                    }
                }

                (void)hash160.Write(data);
                (void)hash256.Write(data);
                (void)hmac_sha256.Write(data.data(), data.size());
                (void)hmac_sha512.Write(data.data(), data.size());
                (void)ripemd160.Write(data.data(), data.size());
                (void)sha1.Write(data.data(), data.size());
                (void)sha256.Write(data.data(), data.size());
                (void)sha3.Write(data);
                (void)sha512.Write(data.data(), data.size());
                (void)sip_hasher.Write(data);

                (void)Hash(data);
                (void)Hash160(data);
                (void)sha512.Size();
            },
            [&] {
                (void)hash160.Reset();
                (void)hash256.Reset();
                (void)ripemd160.Reset();
                (void)sha1.Reset();
                (void)sha256.Reset();
                (void)sha3.Reset();
                (void)sha512.Reset();
            },
            [&] {
                CallOneOf(
                    fuzzed_data_provider,
                    [&] {
                        data.resize(CHash160::OUTPUT_SIZE);
                        hash160.Finalize(data);
                    },
                    [&] {
                        data.resize(CHash256::OUTPUT_SIZE);
                        hash256.Finalize(data);
                    },
                    [&] {
                        data.resize(CHMAC_SHA256::OUTPUT_SIZE);
                        hmac_sha256.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CHMAC_SHA512::OUTPUT_SIZE);
                        hmac_sha512.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CRIPEMD160::OUTPUT_SIZE);
                        ripemd160.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA1::OUTPUT_SIZE);
                        sha1.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA256::OUTPUT_SIZE);
                        sha256.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA512::OUTPUT_SIZE);
                        sha512.Finalize(data.data());
                    },
                    [&] {
                        data.resize(1);
                        data[0] = sip_hasher.Finalize() % 256;
                    },
                    [&] {
                        data.resize(SHA3_256::OUTPUT_SIZE);
                        sha3.Finalize(data);
                    });
            });
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        uint64_t state[25];
        for (size_t i = 0; i < 25; ++i) {
            state[i] = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
        }
        KeccakF(state);
    }
}
