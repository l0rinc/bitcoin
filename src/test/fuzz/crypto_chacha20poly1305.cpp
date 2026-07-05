// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>
#include <random.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct FSChaCha20Poly1305Model {
    std::vector<std::byte> key;
    uint32_t packet_counter{0};
    uint64_t rekey_counter{0};

    void NextPacket(uint32_t rekey_interval)
    {
        assert(rekey_interval > 0);
        assert(packet_counter < rekey_interval);
        if (++packet_counter == rekey_interval) {
            std::array<std::byte, ChaCha20Aligned::BLOCKLEN> next_key_material;
            AEADChaCha20Poly1305 aead{key};
            aead.Keystream({0xffffffff, rekey_counter}, next_key_material);
            key.assign(next_key_material.begin(), next_key_material.begin() + FSChaCha20Poly1305::KEYLEN);
            packet_counter = 0;
            ++rekey_counter;
        }
        assert(packet_counter < rekey_interval);
    }
};

static inline void crypt_packets(FSChaCha20Poly1305& aead, FSChaCha20Poly1305Model& model, uint32_t rekey_interval, uint32_t packets, bool encrypt)
{
    for (uint32_t i = 0; i < packets; ++i) {
        std::byte dummy_tag[FSChaCha20Poly1305::EXPANSION] = {{}};
        if (encrypt) {
            aead.Encrypt(std::span{dummy_tag}.first(0), std::span{dummy_tag}.first(0), dummy_tag);
        } else {
            aead.Decrypt(dummy_tag, std::span{dummy_tag}.first(0), std::span{dummy_tag}.first(0));
        }
        model.NextPacket(rekey_interval);
    }
}

void AssertFailedDecryptLeftOutputUntouched(bool ok, const std::vector<std::byte>& before, const std::vector<std::byte>& after)
{
    assert(!ok);
    assert(before == after);
}

FUZZ_TARGET(crypto_aeadchacha20poly1305)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    auto key = provider.ConsumeBytes<std::byte>(32);
    key.resize(32);
    AEADChaCha20Poly1305 aead(key);

    // Initialize RNG deterministically, to generate contents and AAD. We assume that there are no
    // (potentially buggy) edge cases triggered by specific values of contents/AAD, so we can avoid
    // reading the actual data for those from the fuzzer input (which would need large amounts of
    // data).
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    LIMITED_WHILE (provider.ConsumeBool(), 100) {
        // Mode:
        // - Bit 0: whether to use single-plain Encrypt/Decrypt; otherwise use a split at prefix.
        // - Bit 2: whether this ciphertext will be corrupted (making it the last sent one)
        // - Bit 3-4: controls the maximum aad length (max 511 bytes)
        // - Bit 5-7: controls the maximum content length (max 16383 bytes, for performance reasons)
        unsigned mode = provider.ConsumeIntegral<uint8_t>();
        bool use_splits = mode & 1;
        bool damage = mode & 4;
        unsigned aad_length_bits = 3 * ((mode >> 3) & 3);
        unsigned aad_length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << aad_length_bits) - 1);
        unsigned length_bits = 2 * ((mode >> 5) & 7);
        unsigned length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << length_bits) - 1);
        // Generate aad and content.
        auto aad = rng.randbytes<std::byte>(aad_length);
        auto plain = rng.randbytes<std::byte>(length);
        std::vector<std::byte> cipher(length + AEADChaCha20Poly1305::EXPANSION);
        // Generate nonce
        AEADChaCha20Poly1305::Nonce96 nonce = {(uint32_t)rng(), rng()};

        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            aead.Encrypt(std::span{plain}.first(split_index), std::span{plain}.subspan(split_index), aad, nonce, cipher);
        } else {
            aead.Encrypt(plain, aad, nonce, cipher);
        }

        // Test Keystream output
        std::vector<std::byte> keystream(length);
        aead.Keystream(nonce, keystream);
        for (size_t i = 0; i < length; ++i) {
            assert((plain[i] ^ keystream[i]) == cipher[i]);
        }

        std::vector<std::byte> decrypted_contents(length, std::byte{0xa5});
        bool ok{false};

        // damage the key
        unsigned key_position = provider.ConsumeIntegralInRange<unsigned>(0, 31);
        std::byte damage_val{(uint8_t)(1U << (key_position & 7))};
        std::vector<std::byte> bad_key = key;
        bad_key[key_position] ^= damage_val;

        AEADChaCha20Poly1305 bad_aead(bad_key);
        const std::vector<std::byte> failed_key_decrypt_contents{decrypted_contents};
        ok = bad_aead.Decrypt(cipher, aad, nonce, decrypted_contents);
        AssertFailedDecryptLeftOutputUntouched(ok, failed_key_decrypt_contents, decrypted_contents);

        // Optionally damage 1 bit in either the cipher (corresponding to a change in transit)
        // or the aad (to make sure that decryption will fail if the AAD mismatches).
        if (damage) {
            unsigned damage_bit = provider.ConsumeIntegralInRange<unsigned>(0, (cipher.size() + aad.size()) * 8U - 1U);
            unsigned damage_pos = damage_bit >> 3;
            std::byte damage_val{(uint8_t)(1U << (damage_bit & 7))};
            if (damage_pos >= cipher.size()) {
                aad[damage_pos - cipher.size()] ^= damage_val;
            } else {
                cipher[damage_pos] ^= damage_val;
            }
        }

        const std::vector<std::byte> before_decrypt_contents(length, std::byte{0x3c});
        decrypted_contents = before_decrypt_contents;
        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            ok = aead.Decrypt(cipher, aad, nonce, std::span{decrypted_contents}.first(split_index), std::span{decrypted_contents}.subspan(split_index));
        } else {
            ok = aead.Decrypt(cipher, aad, nonce, decrypted_contents);
        }

        // Decryption *must* fail if the packet was damaged, and succeed if it wasn't.
        assert(!ok == damage);
        if (!ok) {
            assert(decrypted_contents == before_decrypt_contents);
            break;
        }
        assert(decrypted_contents == plain);
    }
}

FUZZ_TARGET(crypto_fschacha20poly1305)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    uint32_t rekey_interval = provider.ConsumeIntegralInRange<size_t>(32, 512);
    auto key = provider.ConsumeBytes<std::byte>(32);
    key.resize(32);
    FSChaCha20Poly1305 enc_aead(key, rekey_interval);
    FSChaCha20Poly1305 dec_aead(key, rekey_interval);
    FSChaCha20Poly1305Model enc_model{key};
    FSChaCha20Poly1305Model dec_model{key};

    // Initialize RNG deterministically, to generate contents and AAD. We assume that there are no
    // (potentially buggy) edge cases triggered by specific values of contents/AAD, so we can avoid
    // reading the actual data for those from the fuzzer input (which would need large amounts of
    // data).
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    LIMITED_WHILE (provider.ConsumeBool(), 100) {
        // Mode:
        // - Bit 0: whether to use single-plain Encrypt/Decrypt; otherwise use a split at prefix.
        // - Bit 2: whether this ciphertext will be corrupted (making it the last sent one)
        // - Bit 3-4: controls the maximum aad length (max 511 bytes)
        // - Bit 5-7: controls the maximum content length (max 16383 bytes, for performance reasons)
        unsigned mode = provider.ConsumeIntegral<uint8_t>();
        bool use_splits = mode & 1;
        bool damage = mode & 4;
        unsigned aad_length_bits = 3 * ((mode >> 3) & 3);
        unsigned aad_length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << aad_length_bits) - 1);
        unsigned length_bits = 2 * ((mode >> 5) & 7);
        unsigned length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << length_bits) - 1);
        // Generate aad and content.
        auto aad = rng.randbytes<std::byte>(aad_length);
        auto plain = rng.randbytes<std::byte>(length);
        std::vector<std::byte> cipher(length + FSChaCha20Poly1305::EXPANSION);
        const uint32_t advance_packets{provider.ConsumeIntegralInRange<uint32_t>(0, rekey_interval * 2)};

        crypt_packets(enc_aead, enc_model, rekey_interval, advance_packets, true);
        std::vector<std::byte> reference_cipher(cipher.size());
        AEADChaCha20Poly1305 reference_enc{enc_model.key};
        reference_enc.Encrypt(plain, aad, {enc_model.packet_counter, enc_model.rekey_counter}, reference_cipher);
        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            enc_aead.Encrypt(std::span{plain}.first(split_index), std::span{plain}.subspan(split_index), aad, cipher);
        } else {
            enc_aead.Encrypt(plain, aad, cipher);
        }
        assert(cipher == reference_cipher);
        enc_model.NextPacket(rekey_interval);

        std::vector<std::byte> decrypted_contents(length, std::byte{0xa5});
        bool ok{false};

        // damage the key
        unsigned key_position = provider.ConsumeIntegralInRange<unsigned>(0, 31);
        std::byte damage_val{(uint8_t)(1U << (key_position & 7))};
        std::vector<std::byte> bad_key = key;
        bad_key[key_position] ^= damage_val;

        FSChaCha20Poly1305 bad_fs_aead(bad_key, rekey_interval);
        FSChaCha20Poly1305Model bad_model{bad_key};
        crypt_packets(bad_fs_aead, bad_model, rekey_interval, advance_packets, false);
        const std::vector<std::byte> failed_key_decrypt_contents{decrypted_contents};
        ok = bad_fs_aead.Decrypt(cipher, aad, decrypted_contents);
        AssertFailedDecryptLeftOutputUntouched(ok, failed_key_decrypt_contents, decrypted_contents);

        // Optionally damage 1 bit in either the cipher (corresponding to a change in transit)
        // or the aad (to make sure that decryption will fail if the AAD mismatches).
        if (damage) {
            unsigned damage_bit = provider.ConsumeIntegralInRange<unsigned>(0, (cipher.size() + aad.size()) * 8U - 1U);
            unsigned damage_pos = damage_bit >> 3;
            std::byte damage_val{(uint8_t)(1U << (damage_bit & 7))};
            if (damage_pos >= cipher.size()) {
                aad[damage_pos - cipher.size()] ^= damage_val;
            } else {
                cipher[damage_pos] ^= damage_val;
            }
        }

        const std::vector<std::byte> before_decrypt_contents(length, std::byte{0x3c});
        decrypted_contents = before_decrypt_contents;
        crypt_packets(dec_aead, dec_model, rekey_interval, advance_packets, false);
        AEADChaCha20Poly1305 reference_dec{dec_model.key};
        std::vector<std::byte> reference_plain(length, std::byte{0xa5});
        const bool reference_ok{reference_dec.Decrypt(cipher, aad, {dec_model.packet_counter, dec_model.rekey_counter}, reference_plain)};
        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            ok = dec_aead.Decrypt(cipher, aad, std::span{decrypted_contents}.first(split_index), std::span{decrypted_contents}.subspan(split_index));
        } else {
            ok = dec_aead.Decrypt(cipher, aad, decrypted_contents);
        }
        dec_model.NextPacket(rekey_interval);

        // Decryption *must* fail if the packet was damaged, and succeed if it wasn't.
        assert(ok == reference_ok);
        assert(!ok == damage);
        if (!ok) {
            assert(decrypted_contents == before_decrypt_contents);
            break;
        }
        assert(reference_plain == plain);
        assert(decrypted_contents == plain);
    }
}
