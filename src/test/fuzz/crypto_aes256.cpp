// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/aes.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cassert>
#include <cstdint>

FUZZ_TARGET(crypto_aes256)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const auto key{ConsumeFixedLengthByteArray<AES256_KEYSIZE>(fuzzed_data_provider)};

    AES256Encrypt encrypt{key.data()};
    AES256Decrypt decrypt{key.data()};

    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const auto plaintext{ConsumeFixedLengthByteArray<AES_BLOCKSIZE>(fuzzed_data_provider)};
        std::array<uint8_t, AES_BLOCKSIZE> ciphertext{};
        encrypt.Encrypt(ciphertext.data(), plaintext.data());
        std::array<uint8_t, AES_BLOCKSIZE> decrypted_plaintext{};
        decrypt.Decrypt(decrypted_plaintext.data(), ciphertext.data());
        assert(decrypted_plaintext == plaintext);
    }
}
