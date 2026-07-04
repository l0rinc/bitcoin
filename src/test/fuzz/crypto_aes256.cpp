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
#include <vector>

namespace {
constexpr size_t GUARD_SIZE{8};
constexpr uint8_t INPUT_GUARD{0xa5};
constexpr uint8_t OUTPUT_GUARD{0x3c};

using GuardedBlock = std::array<uint8_t, AES_BLOCKSIZE + 2 * GUARD_SIZE>;

GuardedBlock MakeGuardedBlock(const std::vector<uint8_t>& block, uint8_t guard)
{
    assert(block.size() == AES_BLOCKSIZE);
    GuardedBlock guarded;
    guarded.fill(guard);
    std::copy(block.begin(), block.end(), guarded.begin() + GUARD_SIZE);
    return guarded;
}

bool GuardBytesUntouched(const GuardedBlock& block, uint8_t guard)
{
    return std::all_of(block.begin(), block.begin() + GUARD_SIZE, [guard](uint8_t value) { return value == guard; }) &&
           std::all_of(block.end() - GUARD_SIZE, block.end(), [guard](uint8_t value) { return value == guard; });
}

bool BlockEquals(const GuardedBlock& guarded, const std::vector<uint8_t>& block)
{
    assert(block.size() == AES_BLOCKSIZE);
    return std::equal(block.begin(), block.end(), guarded.begin() + GUARD_SIZE);
}
} // namespace

FUZZ_TARGET(crypto_aes256)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> key = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES256_KEYSIZE);

    AES256Encrypt encrypt{key.data()};
    AES256Decrypt decrypt{key.data()};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const std::vector<uint8_t> plaintext = ConsumeFixedLengthByteVector(fuzzed_data_provider, AES_BLOCKSIZE);
        GuardedBlock guarded_plaintext{MakeGuardedBlock(plaintext, INPUT_GUARD)};
        const GuardedBlock guarded_plaintext_before{guarded_plaintext};

        GuardedBlock guarded_ciphertext;
        guarded_ciphertext.fill(OUTPUT_GUARD);
        encrypt.Encrypt(guarded_ciphertext.data() + GUARD_SIZE, guarded_plaintext.data() + GUARD_SIZE);
        assert(guarded_plaintext == guarded_plaintext_before);
        assert(GuardBytesUntouched(guarded_ciphertext, OUTPUT_GUARD));

        std::vector<uint8_t> ciphertext(AES_BLOCKSIZE);
        std::copy(guarded_ciphertext.begin() + GUARD_SIZE, guarded_ciphertext.begin() + GUARD_SIZE + AES_BLOCKSIZE, ciphertext.begin());
        GuardedBlock guarded_decrypted;
        guarded_decrypted.fill(OUTPUT_GUARD);
        const GuardedBlock guarded_ciphertext_before{guarded_ciphertext};
        decrypt.Decrypt(guarded_decrypted.data() + GUARD_SIZE, guarded_ciphertext.data() + GUARD_SIZE);
        assert(guarded_ciphertext == guarded_ciphertext_before);
        assert(GuardBytesUntouched(guarded_decrypted, OUTPUT_GUARD));
        assert(BlockEquals(guarded_decrypted, plaintext));

        GuardedBlock guarded_inplace{MakeGuardedBlock(ciphertext, OUTPUT_GUARD)};
        decrypt.Decrypt(guarded_inplace.data() + GUARD_SIZE, guarded_inplace.data() + GUARD_SIZE);
        assert(GuardBytesUntouched(guarded_inplace, OUTPUT_GUARD));
        assert(BlockEquals(guarded_inplace, plaintext));
    }
}
