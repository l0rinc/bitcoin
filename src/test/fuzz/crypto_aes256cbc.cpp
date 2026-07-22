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
void AssertAES256CBCVector(std::string_view key_hex, std::string_view iv_hex, std::string_view plaintext_hex,
                           std::string_view ciphertext_hex, bool pad)
{
    const std::vector<unsigned char> key{ParseHex(std::string{key_hex})};
    const std::vector<unsigned char> iv{ParseHex(std::string{iv_hex})};
    const std::vector<unsigned char> plaintext{ParseHex(std::string{plaintext_hex})};
    const std::vector<unsigned char> expected_ciphertext{ParseHex(std::string{ciphertext_hex})};
    assert(key.size() == AES256_KEYSIZE);
    assert(iv.size() == AES_BLOCKSIZE);
    assert(expected_ciphertext.size() % AES_BLOCKSIZE == 0);

    AES256CBCEncrypt encrypt{key.data(), iv.data(), pad};
    std::vector<unsigned char> ciphertext(expected_ciphertext.size() + AES_BLOCKSIZE, 0xA5);
    const int encrypt_ret = encrypt.Encrypt(plaintext.data(), plaintext.size(), ciphertext.data());
    assert(encrypt_ret >= 0);
    assert(static_cast<size_t>(encrypt_ret) == expected_ciphertext.size());
    assert(std::equal(ciphertext.begin(), ciphertext.begin() + encrypt_ret, expected_ciphertext.begin()));
    assert(std::all_of(ciphertext.begin() + encrypt_ret, ciphertext.end(), [](const unsigned char byte) { return byte == 0xA5; }));

    std::vector<unsigned char> repeated_ciphertext(ciphertext.size(), 0x5A);
    assert(encrypt.Encrypt(plaintext.data(), plaintext.size(), repeated_ciphertext.data()) == encrypt_ret);
    assert(std::equal(repeated_ciphertext.begin(), repeated_ciphertext.begin() + encrypt_ret, ciphertext.begin()));

    AES256CBCDecrypt decrypt{key.data(), iv.data(), pad};
    std::vector<unsigned char> decrypted(expected_ciphertext.size(), 0xA5);
    const int decrypt_ret = decrypt.Decrypt(ciphertext.data(), encrypt_ret, decrypted.data());
    assert(decrypt_ret >= 0);
    assert(static_cast<size_t>(decrypt_ret) == plaintext.size());
    assert(std::equal(decrypted.begin(), decrypted.begin() + decrypt_ret, plaintext.begin()));
}

void AssertAES256CBCVectors()
{
    constexpr std::string_view key{
        "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4"};
    constexpr std::string_view iv{"000102030405060708090a0b0c0d0e0f"};
    constexpr std::string_view first_block{"6bc1bee22e409f96e93d7e117393172a"};
    constexpr std::string_view plaintext{
        "6bc1bee22e409f96e93d7e117393172a"
        "ae2d8a571e03ac9c9eb76fac45af8e51"
        "30c81c46a35ce411e5fbc1191a0a52ef"
        "f69f2445df4f9b17ad2b417be66c3710"};
    constexpr std::string_view ciphertext{
        "f58c4c04d6e5f1ba779eabfb5f7bfbd6"
        "9cfc4e967edb808d679f777bc6702c7d"
        "39f23369a9d9bacfa530e26304231461"
        "b2eb05e2c39be9fcda6c19078c6a9d1b"};
    constexpr std::string_view padded_ciphertext{
        "f58c4c04d6e5f1ba779eabfb5f7bfbd6"
        "485a5c81519cf378fa36d42b8547edc0"};

    AssertAES256CBCVector(key, iv, plaintext, ciphertext, /*pad=*/false);
    AssertAES256CBCVector(key, iv, first_block, padded_ciphertext, /*pad=*/true);
}

void AssertAES256CBCBoundaries()
{
    const std::array<unsigned char, AES256_KEYSIZE> key{};
    const std::array<unsigned char, AES_BLOCKSIZE> iv{};
    constexpr std::array<size_t, 9> sizes{0, 1, 15, 16, 17, 31, 32, 33, 64};

    for (const size_t size : sizes) {
        std::vector<unsigned char> plaintext(size);
        for (size_t i = 0; i != plaintext.size(); ++i)
            plaintext[i] = static_cast<unsigned char>(i * 37 + 11);

        for (const bool pad : {false, true}) {
            AES256CBCEncrypt encrypt{key.data(), iv.data(), pad};
            const size_t expected_size = pad ? (size == 0 ? 0 : ((size / AES_BLOCKSIZE) + 1) * AES_BLOCKSIZE) : (size != 0 && size % AES_BLOCKSIZE == 0 ? size : 0);
            std::vector<unsigned char> ciphertext(size + AES_BLOCKSIZE, 0xA5);
            const int encrypt_ret = encrypt.Encrypt(plaintext.data(), plaintext.size(), ciphertext.data());
            assert(encrypt_ret >= 0);
            assert(static_cast<size_t>(encrypt_ret) == expected_size);
            if (encrypt_ret == 0)
                continue;

            AES256CBCDecrypt decrypt{key.data(), iv.data(), pad};
            std::vector<unsigned char> decrypted(ciphertext.size(), 0xA5);
            const int decrypt_ret = decrypt.Decrypt(ciphertext.data(), encrypt_ret, decrypted.data());
            assert(decrypt_ret >= 0);
            assert(static_cast<size_t>(decrypt_ret) == plaintext.size());
            assert(std::equal(decrypted.begin(), decrypted.begin() + decrypt_ret, plaintext.begin()));

            if (pad) {
                ciphertext.resize(encrypt_ret);
                ciphertext.back() = 0;
                assert(decrypt.Decrypt(ciphertext.data(), ciphertext.size(), decrypted.data()) == 0);
                assert(decrypt.Decrypt(ciphertext.data(), ciphertext.size() - 1, decrypted.data()) == 0);
            }
        }
    }
}
} // namespace

FUZZ_TARGET(crypto_aes256cbc)
{
    AssertAES256CBCVectors();
    AssertAES256CBCBoundaries();

    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);
    const std::vector<uint8_t> iv = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
    const bool pad = fuzzed_data_provider.ConsumeBool();

    AES256CBCEncrypt encrypt{key.data(), iv.data(), pad};
    AES256CBCDecrypt decrypt{key.data(), iv.data(), pad};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCKSIZE);
        const int encrypt_ret = encrypt.Encrypt(plaintext.data(), plaintext.size(), ciphertext.data());
        ciphertext.resize(encrypt_ret);
        std::vector<uint8_t> decrypted_plaintext(ciphertext.size());
        const int decrypt_ret = decrypt.Decrypt(ciphertext.data(), ciphertext.size(), decrypted_plaintext.data());
        decrypted_plaintext.resize(decrypt_ret);
        assert(decrypted_plaintext == plaintext || (!pad && plaintext.size() % AES_BLOCKSIZE != 0 && encrypt_ret == 0 && decrypt_ret == 0));
    }
}
