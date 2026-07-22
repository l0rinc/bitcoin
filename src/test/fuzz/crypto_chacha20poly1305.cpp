// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>
#include <random.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

class FSChaCha20Poly1305Model
{
    AEADChaCha20Poly1305 m_aead;
    const uint32_t m_rekey_interval;
    uint32_t m_packet_counter{0};
    uint64_t m_rekey_counter{0};

    void NextPacket() noexcept
    {
        assert(m_rekey_interval != 0);
        assert(m_packet_counter < m_rekey_interval);
        if (++m_packet_counter == m_rekey_interval) {
            std::array<std::byte, 64> one_block{};
            m_aead.Keystream({0xFFFFFFFF, m_rekey_counter}, one_block);
            m_aead.SetKey(std::span{one_block}.first(AEADChaCha20Poly1305::KEYLEN));
            m_packet_counter = 0;
            ++m_rekey_counter;
        }
    }

public:
    FSChaCha20Poly1305Model(std::span<const std::byte> key, uint32_t rekey_interval) noexcept : m_aead(key), m_rekey_interval(rekey_interval)
    {
        assert(rekey_interval != 0);
    }

    void Encrypt(std::span<const std::byte> plain1, std::span<const std::byte> plain2,
                 std::span<const std::byte> aad, std::span<std::byte> cipher) noexcept
    {
        assert(m_rekey_interval != 0);
        assert(m_packet_counter < m_rekey_interval);
        m_aead.Encrypt(plain1, plain2, aad, {m_packet_counter, m_rekey_counter}, cipher);
        NextPacket();
    }

    void Encrypt(std::span<const std::byte> plain, std::span<const std::byte> aad, std::span<std::byte> cipher) noexcept
    {
        Encrypt(plain, {}, aad, cipher);
    }

    bool Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad,
                 std::span<std::byte> plain1, std::span<std::byte> plain2) noexcept
    {
        assert(m_rekey_interval != 0);
        assert(m_packet_counter < m_rekey_interval);
        const bool ret = m_aead.Decrypt(cipher, aad, {m_packet_counter, m_rekey_counter}, plain1, plain2);
        NextPacket();
        return ret;
    }

    bool Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad, std::span<std::byte> plain) noexcept
    {
        return Decrypt(cipher, aad, plain, {});
    }
};

template <typename Actual, typename Model>
static inline void crypt_till_rekey(Actual& actual, Model& model, uint32_t rekey_interval, bool encrypt)
{
    for (uint32_t i = 0; i < rekey_interval; ++i) {
        std::byte dummy_tag[FSChaCha20Poly1305::EXPANSION] = {{}};
        if (encrypt) {
            actual.Encrypt(std::span{dummy_tag}.first(0), std::span{dummy_tag}.first(0), dummy_tag);
            model.Encrypt(std::span{dummy_tag}.first(0), std::span{dummy_tag}.first(0), dummy_tag);
        } else {
            actual.Decrypt(dummy_tag, std::span{dummy_tag}.first(0), std::span{dummy_tag}.first(0));
            model.Decrypt(dummy_tag, std::span{dummy_tag}.first(0), std::span{dummy_tag}.first(0));
        }
    }
}

FUZZ_TARGET(crypto_aeadchacha20poly1305)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};

    auto key = provider.ConsumeBytes<std::byte>(32);
    key.resize(32);
    AEADChaCha20Poly1305 aead(key);

    // Initialize the deterministic RNG used for random contents and AAD.
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

        std::vector<std::byte> decrypted_contents(length);
        bool ok{false};

        // damage the key
        unsigned key_position = provider.ConsumeIntegralInRange<unsigned>(0, 31);
        std::byte damage_val{(uint8_t)(1U << (key_position & 7))};
        std::vector<std::byte> bad_key = key;
        bad_key[key_position] ^= damage_val;

        AEADChaCha20Poly1305 bad_aead(bad_key);
        ok = bad_aead.Decrypt(cipher, aad, nonce, decrypted_contents);
        assert(!ok);

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

        if (use_splits && length > 0) {
            size_t split_index = provider.ConsumeIntegralInRange<size_t>(1, length);
            ok = aead.Decrypt(cipher, aad, nonce, std::span{decrypted_contents}.first(split_index), std::span{decrypted_contents}.subspan(split_index));
        } else {
            ok = aead.Decrypt(cipher, aad, nonce, decrypted_contents);
        }

        // Decryption *must* fail if the packet was damaged, and succeed if it wasn't.
        assert(!ok == damage);
        if (!ok) break;
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
    FSChaCha20Poly1305Model enc_model(key, rekey_interval);
    FSChaCha20Poly1305Model dec_model(key, rekey_interval);

    constexpr size_t NO_SPLIT = std::numeric_limits<size_t>::max();
    constexpr size_t NO_DAMAGE = std::numeric_limits<size_t>::max();
    constexpr size_t NO_BAD_KEY = std::numeric_limits<size_t>::max();

    auto check_packet = [&](std::span<const std::byte> plain, std::span<const std::byte> aad,
                            size_t encrypt_split, size_t decrypt_split, size_t damage_bit,
                            size_t bad_key_position) {
        std::vector<std::byte> cipher(plain.size() + FSChaCha20Poly1305::EXPANSION);
        std::vector<std::byte> model_cipher(cipher.size());

        if (encrypt_split != NO_SPLIT) {
            enc_aead.Encrypt(plain.first(encrypt_split), plain.subspan(encrypt_split), aad, cipher);
            enc_model.Encrypt(plain.first(encrypt_split), plain.subspan(encrypt_split), aad, model_cipher);
        } else {
            enc_aead.Encrypt(plain, aad, cipher);
            enc_model.Encrypt(plain, aad, model_cipher);
        }
        assert(cipher == model_cipher);

        if (bad_key_position != NO_BAD_KEY) {
            std::vector<std::byte> bad_key = key;
            bad_key[bad_key_position] ^= std::byte{uint8_t(1U << (bad_key_position & 7))};
            FSChaCha20Poly1305 bad_aead(bad_key, rekey_interval);
            FSChaCha20Poly1305Model bad_model(bad_key, rekey_interval);
            crypt_till_rekey(bad_aead, bad_model, rekey_interval, false);
            std::vector<std::byte> bad_plain(plain.size());
            const bool bad_ok = bad_aead.Decrypt(cipher, aad, bad_plain);
            const bool bad_model_ok = bad_model.Decrypt(cipher, aad, bad_plain);
            assert(!bad_ok);
            assert(bad_ok == bad_model_ok);
        }

        std::vector<std::byte> decrypt_aad(aad.begin(), aad.end());
        if (damage_bit != NO_DAMAGE) {
            assert(damage_bit < (cipher.size() + decrypt_aad.size()) * 8U);
            const size_t damage_pos = damage_bit >> 3;
            const std::byte damage_value{uint8_t(1U << (damage_bit & 7))};
            if (damage_pos >= cipher.size()) {
                decrypt_aad[damage_pos - cipher.size()] ^= damage_value;
            } else {
                cipher[damage_pos] ^= damage_value;
            }
        }

        std::vector<std::byte> decrypted_contents(plain.size());
        std::vector<std::byte> model_decrypted_contents(plain.size());
        bool ok{false};
        bool model_ok{false};
        if (decrypt_split != NO_SPLIT) {
            ok = dec_aead.Decrypt(cipher, decrypt_aad, std::span{decrypted_contents}.first(decrypt_split), std::span{decrypted_contents}.subspan(decrypt_split));
            model_ok = dec_model.Decrypt(cipher, decrypt_aad, std::span{model_decrypted_contents}.first(decrypt_split), std::span{model_decrypted_contents}.subspan(decrypt_split));
        } else {
            ok = dec_aead.Decrypt(cipher, decrypt_aad, decrypted_contents);
            model_ok = dec_model.Decrypt(cipher, decrypt_aad, model_decrypted_contents);
        }

        assert(ok == model_ok);
        assert(!ok == (damage_bit != NO_DAMAGE));
        if (!ok) return;
        assert(decrypted_contents == model_decrypted_contents);
        assert(std::equal(decrypted_contents.begin(), decrypted_contents.end(), plain.begin(), plain.end()));
    };

    // Force non-empty packets immediately before and after two rekey transitions. This keeps the
    // oracle useful even when the fuzzer input does not enter the random-operation loop.
    std::array<std::byte, 32> boundary_aad{};
    for (size_t i = 0; i < boundary_aad.size(); ++i)
        boundary_aad[i] = std::byte{uint8_t(i)};
    const std::span<const std::byte> boundary_aad_span{boundary_aad};
    constexpr std::array<size_t, 9> boundary_lengths{0, 1, 63, 64, 65, 127, 128, 129, 4096};
    for (int cycle = 0; cycle < 2; ++cycle) {
        for (uint32_t i = 0; i < rekey_interval - 2; ++i) {
            check_packet(std::span<const std::byte>{}, std::span<const std::byte>{}, NO_SPLIT, NO_SPLIT, NO_DAMAGE, NO_BAD_KEY);
        }
        for (size_t length : boundary_lengths) {
            std::vector<std::byte> plain(length, std::byte{uint8_t(length)});
            const auto aad = boundary_aad_span.first(length % (boundary_aad.size() + 1));
            const size_t split = length > 1 ? length / 2 : NO_SPLIT;
            check_packet(plain, aad, split, split, NO_DAMAGE, NO_BAD_KEY);
        }
    }

    // Generate random contents and AAD deterministically after the fixed boundary cases above.
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

        crypt_till_rekey(enc_aead, enc_model, rekey_interval, true);
        size_t encrypt_split = NO_SPLIT;
        size_t decrypt_split = NO_SPLIT;
        crypt_till_rekey(dec_aead, dec_model, rekey_interval, false);
        if (use_splits && length > 0) {
            encrypt_split = provider.ConsumeIntegralInRange<size_t>(1, length);
            decrypt_split = provider.ConsumeIntegralInRange<size_t>(1, length);
        }
        const size_t damage_bit = damage ? provider.ConsumeIntegralInRange<size_t>(0, (plain.size() + FSChaCha20Poly1305::EXPANSION + aad.size()) * 8U - 1U) : NO_DAMAGE;
        const size_t bad_key_position = provider.ConsumeIntegralInRange<size_t>(0, 31);
        check_packet(plain, aad, encrypt_split, decrypt_split, damage_bit, bad_key_position);
        if (damage) break;
    }
}
