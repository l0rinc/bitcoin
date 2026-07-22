// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>
#include <crypto/common.h>
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

class AEADChaCha20Poly1305Model
{
    ChaCha20 m_chacha20;

    static void ComputeTag(ChaCha20& chacha20, std::span<const std::byte> aad,
                           std::span<const std::byte> cipher, std::span<std::byte> tag) noexcept
    {
        assert(tag.size() == Poly1305::TAGLEN);
        std::array<std::byte, 16> padding{};
        std::array<std::byte, ChaCha20Aligned::BLOCKLEN> first_block{};
        chacha20.Keystream(first_block);

        Poly1305 poly1305{std::span{first_block}.first(Poly1305::KEYLEN)};
        const size_t aad_padding_length = (16 - (aad.size() % 16)) % 16;
        poly1305.Update(aad).Update(std::span{padding}.first(aad_padding_length));
        const size_t cipher_padding_length = (16 - (cipher.size() % 16)) % 16;
        poly1305.Update(cipher).Update(std::span{padding}.first(cipher_padding_length));

        std::array<std::byte, Poly1305::TAGLEN> length_desc{};
        WriteLE64(length_desc.data(), aad.size());
        WriteLE64(length_desc.data() + 8, cipher.size());
        poly1305.Update(length_desc).Finalize(tag);
    }

public:
    explicit AEADChaCha20Poly1305Model(std::span<const std::byte> key) noexcept : m_chacha20(key)
    {
        assert(key.size() == ChaCha20::KEYLEN);
    }

    void Encrypt(std::span<const std::byte> plain1, std::span<const std::byte> plain2,
                 std::span<const std::byte> aad, ChaCha20::Nonce96 nonce,
                 std::span<std::byte> cipher) noexcept
    {
        assert(cipher.size() == plain1.size() + plain2.size() + Poly1305::TAGLEN);
        m_chacha20.Seek(nonce, 1);
        m_chacha20.Crypt(plain1, cipher.first(plain1.size()));
        m_chacha20.Crypt(plain2, cipher.subspan(plain1.size()).first(plain2.size()));
        m_chacha20.Seek(nonce, 0);
        ComputeTag(m_chacha20, aad, cipher.first(cipher.size() - Poly1305::TAGLEN), cipher.last(Poly1305::TAGLEN));
    }

    void Encrypt(std::span<const std::byte> plain, std::span<const std::byte> aad,
                 ChaCha20::Nonce96 nonce, std::span<std::byte> cipher) noexcept
    {
        Encrypt(plain, {}, aad, nonce, cipher);
    }

    bool Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad,
                 ChaCha20::Nonce96 nonce, std::span<std::byte> plain1,
                 std::span<std::byte> plain2) noexcept
    {
        assert(cipher.size() == plain1.size() + plain2.size() + Poly1305::TAGLEN);
        m_chacha20.Seek(nonce, 0);
        std::array<std::byte, Poly1305::TAGLEN> expected_tag{};
        ComputeTag(m_chacha20, aad, cipher.first(cipher.size() - Poly1305::TAGLEN), expected_tag);
        if (!std::equal(expected_tag.begin(), expected_tag.end(), cipher.last(Poly1305::TAGLEN).begin())) return false;

        m_chacha20.Seek(nonce, 1);
        m_chacha20.Crypt(cipher.first(plain1.size()), plain1);
        m_chacha20.Crypt(cipher.subspan(plain1.size()).first(plain2.size()), plain2);
        return true;
    }

    bool Decrypt(std::span<const std::byte> cipher, std::span<const std::byte> aad,
                 ChaCha20::Nonce96 nonce, std::span<std::byte> plain) noexcept
    {
        return Decrypt(cipher, aad, nonce, plain, {});
    }

    void Keystream(ChaCha20::Nonce96 nonce, std::span<std::byte> keystream) noexcept
    {
        m_chacha20.Seek(nonce, 1);
        m_chacha20.Keystream(keystream);
    }
};

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

    constexpr size_t NO_SPLIT = std::numeric_limits<size_t>::max();
    constexpr size_t NO_DAMAGE = std::numeric_limits<size_t>::max();
    constexpr size_t NO_BAD_KEY = std::numeric_limits<size_t>::max();

    auto key = provider.ConsumeBytes<std::byte>(32);
    key.resize(32);
    AEADChaCha20Poly1305 enc_aead(key);
    AEADChaCha20Poly1305 dec_aead(key);
    AEADChaCha20Poly1305Model enc_model(key);
    AEADChaCha20Poly1305Model dec_model(key);

    auto check_packet = [&](std::span<const std::byte> plain, std::span<const std::byte> aad,
                            AEADChaCha20Poly1305::Nonce96 nonce, size_t encrypt_split,
                            size_t decrypt_split, size_t damage_bit, size_t bad_key_position) {
        std::vector<std::byte> cipher(plain.size() + AEADChaCha20Poly1305::EXPANSION);
        std::vector<std::byte> model_cipher(cipher.size());

        if (encrypt_split != NO_SPLIT) {
            assert(encrypt_split <= plain.size());
            enc_aead.Encrypt(plain.first(encrypt_split), plain.subspan(encrypt_split), aad, nonce, cipher);
            enc_model.Encrypt(plain.first(encrypt_split), plain.subspan(encrypt_split), aad, nonce, model_cipher);
        } else {
            enc_aead.Encrypt(plain, aad, nonce, cipher);
            enc_model.Encrypt(plain, aad, nonce, model_cipher);
        }
        assert(cipher == model_cipher);

        if (bad_key_position != NO_BAD_KEY) {
            assert(bad_key_position < key.size());
            std::vector<std::byte> bad_key = key;
            bad_key[bad_key_position] ^= std::byte{uint8_t(1U << (bad_key_position & 7))};
            AEADChaCha20Poly1305 bad_aead(bad_key);
            AEADChaCha20Poly1305Model bad_model(bad_key);
            std::vector<std::byte> bad_plain(plain.size());
            std::vector<std::byte> bad_model_plain(plain.size());
            const bool bad_ok = bad_aead.Decrypt(cipher, aad, nonce, bad_plain);
            const bool bad_model_ok = bad_model.Decrypt(cipher, aad, nonce, bad_model_plain);
            assert(!bad_ok);
            assert(bad_ok == bad_model_ok);
        }

        std::vector<std::byte> damaged_cipher = cipher;
        std::vector<std::byte> damaged_aad(aad.begin(), aad.end());
        if (damage_bit != NO_DAMAGE) {
            assert(damage_bit < (damaged_cipher.size() + damaged_aad.size()) * 8U);
            const size_t damage_pos = damage_bit >> 3;
            const std::byte damage_value{uint8_t(1U << (damage_bit & 7))};
            if (damage_pos >= damaged_cipher.size()) {
                damaged_aad[damage_pos - damaged_cipher.size()] ^= damage_value;
            } else {
                damaged_cipher[damage_pos] ^= damage_value;
            }
        }

        std::vector<std::byte> decrypted_contents(plain.size());
        std::vector<std::byte> model_decrypted_contents(plain.size());
        bool ok{false};
        bool model_ok{false};
        if (decrypt_split != NO_SPLIT) {
            assert(decrypt_split <= plain.size());
            ok = dec_aead.Decrypt(damaged_cipher, damaged_aad, nonce,
                                  std::span{decrypted_contents}.first(decrypt_split), std::span{decrypted_contents}.subspan(decrypt_split));
            model_ok = dec_model.Decrypt(damaged_cipher, damaged_aad, nonce,
                                         std::span{model_decrypted_contents}.first(decrypt_split), std::span{model_decrypted_contents}.subspan(decrypt_split));
        } else {
            ok = dec_aead.Decrypt(damaged_cipher, damaged_aad, nonce, decrypted_contents);
            model_ok = dec_model.Decrypt(damaged_cipher, damaged_aad, nonce, model_decrypted_contents);
        }

        assert(ok == model_ok);
        assert(!ok == (damage_bit != NO_DAMAGE));
        if (!ok) return;
        assert(decrypted_contents == model_decrypted_contents);
        assert(std::equal(decrypted_contents.begin(), decrypted_contents.end(), plain.begin(), plain.end()));

        std::vector<std::byte> keystream(plain.size());
        std::vector<std::byte> model_keystream(plain.size());
        AEADChaCha20Poly1305 keystream_aead(key);
        AEADChaCha20Poly1305Model keystream_model(key);
        keystream_aead.Keystream(nonce, keystream);
        keystream_model.Keystream(nonce, model_keystream);
        assert(keystream == model_keystream);
        for (size_t i = 0; i < plain.size(); ++i) {
            assert((plain[i] ^ keystream[i]) == cipher[i]);
        }
    };

    // Exercise the composition around RFC8439's 16-byte padding and ChaCha20's 64-byte blocks,
    // including nonce words at their carry boundaries. This prefix runs even for an empty input.
    constexpr std::array<AEADChaCha20Poly1305::Nonce96, 6> boundary_nonces{{
        {0, 0},
        {0, 1},
        {0, std::numeric_limits<uint64_t>::max()},
        {std::numeric_limits<uint32_t>::max(), 0},
        {std::numeric_limits<uint32_t>::max(), 1},
        {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint64_t>::max()},
    }};
    constexpr std::array<size_t, 10> boundary_plain_lengths{0, 1, 15, 16, 17, 63, 64, 65, 127, 128};
    constexpr std::array<size_t, 9> boundary_aad_lengths{0, 1, 15, 16, 17, 31, 32, 33, 255};
    for (const auto nonce : boundary_nonces) {
        for (const size_t plain_length : boundary_plain_lengths) {
            std::vector<std::byte> plain(plain_length, std::byte{uint8_t(plain_length)});
            for (const size_t aad_length : boundary_aad_lengths) {
                std::vector<std::byte> aad(aad_length);
                for (size_t i = 0; i < aad.size(); ++i)
                    aad[i] = std::byte{uint8_t(i)};
                check_packet(plain, aad, nonce, NO_SPLIT, NO_SPLIT, NO_DAMAGE, NO_BAD_KEY);
                if (plain_length != 0) {
                    const std::array<size_t, 5> splits{0, 1, plain_length / 2, plain_length - 1, plain_length};
                    for (const size_t split : splits) {
                        check_packet(plain, aad, nonce, split, split, NO_DAMAGE, NO_BAD_KEY);
                    }
                }
                if (plain_length == 128 && aad_length == 17) {
                    check_packet(plain, aad, nonce, 1, plain_length - 1, 0, 7);
                    check_packet(plain, aad, nonce, plain_length, 0,
                                 (plain.size() + AEADChaCha20Poly1305::EXPANSION + aad.size()) * 8U - 1U, NO_BAD_KEY);
                }
            }
        }
    }

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
        AEADChaCha20Poly1305::Nonce96 nonce = {(uint32_t)rng(), rng()};
        const size_t encrypt_split = use_splits && length > 0 ? provider.ConsumeIntegralInRange<size_t>(0, length) : NO_SPLIT;
        const size_t decrypt_split = use_splits && length > 0 ? provider.ConsumeIntegralInRange<size_t>(0, length) : NO_SPLIT;
        const size_t damage_bit = damage ? provider.ConsumeIntegralInRange<size_t>(0, (plain.size() + AEADChaCha20Poly1305::EXPANSION + aad.size()) * 8U - 1U) : NO_DAMAGE;
        const size_t bad_key_position = provider.ConsumeIntegralInRange<size_t>(0, 31);
        check_packet(plain, aad, nonce, encrypt_split, decrypt_split, damage_bit, bad_key_position);
        if (damage) break;
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
