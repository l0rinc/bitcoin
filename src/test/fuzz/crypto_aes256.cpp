// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/aes.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {
void AssertAES256Vector(std::string_view key_hex, std::string_view plaintext_hex, std::string_view ciphertext_hex)
{
    const std::vector<unsigned char> key{ParseHex(std::string{key_hex})};
    const std::vector<unsigned char> plaintext{ParseHex(std::string{plaintext_hex})};
    const std::vector<unsigned char> expected_ciphertext{ParseHex(std::string{ciphertext_hex})};
    assert(key.size() == AES256_KEYSIZE);
    assert(plaintext.size() == AES_BLOCKSIZE);
    assert(expected_ciphertext.size() == AES_BLOCKSIZE);

    AES256Encrypt encrypt{key.data()};
    std::array<unsigned char, AES_BLOCKSIZE> ciphertext;
    ciphertext.fill(0xA5);
    encrypt.Encrypt(ciphertext.data(), plaintext.data());
    assert(std::equal(ciphertext.begin(), ciphertext.end(), expected_ciphertext.begin()));

    std::array<unsigned char, AES_BLOCKSIZE> repeated_ciphertext;
    encrypt.Encrypt(repeated_ciphertext.data(), plaintext.data());
    assert(repeated_ciphertext == ciphertext);

    AES256Decrypt decrypt{key.data()};
    std::array<unsigned char, AES_BLOCKSIZE> decrypted;
    decrypt.Decrypt(decrypted.data(), ciphertext.data());
    assert(std::equal(decrypted.begin(), decrypted.end(), plaintext.begin()));

    decrypt.Decrypt(ciphertext.data(), ciphertext.data());
    assert(std::equal(ciphertext.begin(), ciphertext.end(), plaintext.begin()));
}

void AssertAES256Vectors()
{
    AssertAES256Vector(
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
        "00112233445566778899aabbccddeeff",
        "8ea2b7ca516745bfeafc49904b496089");
    AssertAES256Vector(
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
        "6bc1bee22e409f96e93d7e117393172a",
        "f3eed1bdb5d2a03c064b5a7e3db181f8");
    AssertAES256Vector(
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
        "ae2d8a571e03ac9c9eb76fac45af8e51",
        "591ccb10d410ed26dc5ba74a31362870");
    AssertAES256Vector(
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
        "30c81c46a35ce411e5fbc1191a0a52ef",
        "b6ed21b99ca6f4f9f153e7b1beafed1d");
    AssertAES256Vector(
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
        "f69f2445df4f9b17ad2b417be66c3710",
        "23304b7a39f9f3ff067d8d8f9e24ecc7");
}
} // namespace

FUZZ_TARGET(crypto_aes256)
{
    AssertAES256Vectors();

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);

    AES256Encrypt encrypt{key.data()};
    AES256Decrypt decrypt{key.data()};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
        std::vector<uint8_t> ciphertext(AES_BLOCKSIZE);
        encrypt.Encrypt(ciphertext.data(), plaintext.data());
        std::vector<uint8_t> decrypted_plaintext(AES_BLOCKSIZE);
        decrypt.Decrypt(decrypted_plaintext.data(), ciphertext.data());
        assert(decrypted_plaintext == plaintext);
    }
}
