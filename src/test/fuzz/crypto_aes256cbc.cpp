// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/aes.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

FUZZ_TARGET(crypto_aes256cbc)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);
    const std::vector<uint8_t> iv = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
    const bool pad = fuzzed_data_provider.ConsumeBool();

    AES256CBCEncrypt encrypt{key.data(), iv.data(), pad};
    AES256CBCDecrypt decrypt{key.data(), iv.data(), pad};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeRandomLengthByteVector(fuzzed_data_provider);
        assert(plaintext.size() <= static_cast<size_t>(std::numeric_limits<int>::max()));
        const int plaintext_size{static_cast<int>(plaintext.size())};
        std::vector<uint8_t> ciphertext(plaintext.size() + AES_BLOCKSIZE, 0xa5);
        const std::vector<uint8_t> ciphertext_before{ciphertext};
        const int encrypt_ret = encrypt.Encrypt(plaintext.data(), plaintext_size, ciphertext.data());
        const bool encrypt_should_succeed{!plaintext.empty() && (pad || plaintext.size() % AES_BLOCKSIZE == 0)};
        if (encrypt_should_succeed) {
            const int expected_encrypt_ret{pad ? plaintext_size + AES_BLOCKSIZE - (plaintext_size % AES_BLOCKSIZE) : plaintext_size};
            assert(encrypt_ret == expected_encrypt_ret);
            assert(encrypt_ret % AES_BLOCKSIZE == 0);
        } else {
            assert(encrypt_ret == 0);
            assert(ciphertext == ciphertext_before);
        }
        ciphertext.resize(encrypt_ret);
        std::vector<uint8_t> decrypted_plaintext(ciphertext.size(), 0x3c);
        const std::vector<uint8_t> decrypted_plaintext_before{decrypted_plaintext};
        const int decrypt_ret = decrypt.Decrypt(ciphertext.data(), ciphertext.size(), decrypted_plaintext.data());
        if (ciphertext.empty() || (!pad && ciphertext.size() % AES_BLOCKSIZE != 0)) {
            assert(decrypt_ret == 0);
            assert(decrypted_plaintext == decrypted_plaintext_before);
        }
        decrypted_plaintext.resize(decrypt_ret);
        assert(decrypted_plaintext == plaintext || (!encrypt_should_succeed && decrypt_ret == 0));
    }
}
