// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bip324.h>
#include <chainparams.h>
#include <random.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace {

void AssertPacketRoundTrip(BIP324Cipher& sender, BIP324Cipher& receiver,
                           std::span<const std::byte> contents,
                           std::span<const std::byte> aad, bool ignore)
{
    assert(sender);
    assert(receiver);
    std::vector<std::byte> ciphertext(contents.size() + BIP324Cipher::EXPANSION);
    sender.Encrypt(contents, aad, ignore, ciphertext);
    assert(ciphertext.size() == contents.size() + BIP324Cipher::EXPANSION);

    const uint32_t decrypted_length{
        receiver.DecryptLength(std::span{ciphertext}.first(BIP324Cipher::LENGTH_LEN))};
    assert(decrypted_length == contents.size());
    std::vector<std::byte> decrypted(decrypted_length);
    bool decrypted_ignore{false};
    assert(receiver.Decrypt(std::span{ciphertext}.subspan(BIP324Cipher::LENGTH_LEN), aad,
                            decrypted_ignore, decrypted));
    assert(decrypted_ignore == ignore);
    assert(std::ranges::equal(decrypted, contents));
}

void AssertRekeyBoundary(const CKey& init_key, std::span<const std::byte> init_ent,
                         const CKey& resp_key, std::span<const std::byte> resp_ent)
{
    BIP324Cipher initiator(init_key, init_ent);
    BIP324Cipher responder(resp_key, resp_ent);
    assert(!initiator);
    assert(!responder);
    initiator.Initialize(responder.GetOurPubKey(), true);
    responder.Initialize(initiator.GetOurPubKey(), false);
    assert(initiator);
    assert(responder);

    for (const auto [sender, receiver] : {
             std::pair{&initiator, &responder},
             std::pair{&responder, &initiator},
         }) {
        for (unsigned packet{0}; packet <= BIP324Cipher::REKEY_INTERVAL; ++packet) {
            std::vector<std::byte> contents(
                packet % 7, std::byte{static_cast<unsigned char>(packet)});
            std::vector<std::byte> aad(
                packet % 5, std::byte{static_cast<unsigned char>(packet ^ 0xA5)});
            AssertPacketRoundTrip(*sender, *receiver, contents, aad, packet & 1);
        }
    }
}

void Initialize()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::MAIN);
}

}  // namespace

FUZZ_TARGET(bip324_cipher_roundtrip, .init=Initialize)
{
    // Test that BIP324Cipher's encryption and decryption agree.

    // Load keys from fuzzer.
    FuzzedDataProvider provider(buffer.data(), buffer.size());
    // Initiator key
    CKey init_key = ConsumePrivateKey(provider, /*compressed=*/true);
    if (!init_key.IsValid()) return;
    // Initiator entropy
    auto init_ent = provider.ConsumeBytes<std::byte>(32);
    init_ent.resize(32);
    // Responder key
    CKey resp_key = ConsumePrivateKey(provider, /*compressed=*/true);
    if (!resp_key.IsValid()) return;
    // Responder entropy
    auto resp_ent = provider.ConsumeBytes<std::byte>(32);
    resp_ent.resize(32);

    // Initialize ciphers by exchanging public keys.
    BIP324Cipher initiator(init_key, init_ent);
    assert(!initiator);
    BIP324Cipher responder(resp_key, resp_ent);
    assert(!responder);
    initiator.Initialize(responder.GetOurPubKey(), true);
    assert(initiator);
    responder.Initialize(initiator.GetOurPubKey(), false);
    assert(responder);

    // Initialize RNG deterministically, to generate contents and AAD. We assume that there are no
    // (potentially buggy) edge cases triggered by specific values of contents/AAD, so we can avoid
    // reading the actual data for those from the fuzzer input (which would need large amounts of
    // data).
    InsecureRandomContext rng(provider.ConsumeIntegral<uint64_t>());

    // Compare session IDs and garbage terminators.
    assert(std::ranges::equal(initiator.GetSessionID(), responder.GetSessionID()));
    assert(std::ranges::equal(initiator.GetSendGarbageTerminator(), responder.GetReceiveGarbageTerminator()));
    assert(std::ranges::equal(initiator.GetReceiveGarbageTerminator(), responder.GetSendGarbageTerminator()));
    assert(initiator.GetSessionID().size() == BIP324Cipher::SESSION_ID_LEN);
    assert(initiator.GetSendGarbageTerminator().size() == BIP324Cipher::GARBAGE_TERMINATOR_LEN);
    assert(initiator.GetReceiveGarbageTerminator().size() == BIP324Cipher::GARBAGE_TERMINATOR_LEN);

    LIMITED_WHILE (provider.remaining_bytes(), 1000) {
        // Mode:
        // - Bit 0: whether the ignore bit is set in message
        // - Bit 1: whether the responder (0) or initiator (1) sends
        // - Bit 2: whether this ciphertext will be corrupted (making it the last sent one)
        // - Bit 3-4: controls the maximum aad length (max 4095 bytes)
        // - Bit 5-7: controls the maximum content length (max 16383 bytes, for performance reasons)
        unsigned mode = provider.ConsumeIntegral<uint8_t>();
        bool ignore = mode & 1;
        bool from_init = mode & 2;
        bool damage = mode & 4;
        unsigned aad_length_bits = 4 * ((mode >> 3) & 3);
        unsigned aad_length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << aad_length_bits) - 1);
        unsigned length_bits = 2 * ((mode >> 5) & 7);
        unsigned length = provider.ConsumeIntegralInRange<unsigned>(0, (1 << length_bits) - 1);
        // Generate aad and content.
        auto aad = rng.randbytes<std::byte>(aad_length);
        auto contents = rng.randbytes<std::byte>(length);

        // Pick sides.
        auto& sender{from_init ? initiator : responder};
        auto& receiver{from_init ? responder : initiator};

        // Encrypt
        std::vector<std::byte> ciphertext(length + initiator.EXPANSION);
        sender.Encrypt(contents, aad, ignore, ciphertext);

        // Optionally damage 1 bit in either the ciphertext (corresponding to a change in transit)
        // or the aad (to make sure that decryption will fail if the AAD mismatches).
        if (damage) {
            unsigned damage_bit = provider.ConsumeIntegralInRange<unsigned>(0,
                (ciphertext.size() + aad.size()) * 8U - 1U);
            unsigned damage_pos = damage_bit >> 3;
            std::byte damage_val{(uint8_t)(1U << (damage_bit & 7))};
            if (damage_pos >= ciphertext.size()) {
                aad[damage_pos - ciphertext.size()] ^= damage_val;
            } else {
                ciphertext[damage_pos] ^= damage_val;
            }
        }

        // Decrypt length
        uint32_t dec_length = receiver.DecryptLength(std::span{ciphertext}.first(initiator.LENGTH_LEN));
        if (!damage) {
            assert(dec_length == length);
        } else {
            // For performance reasons, don't try to decode if length got increased too much.
            if (dec_length > 16384 + length) break;
            // Otherwise, just append zeros if dec_length > length.
            ciphertext.resize(dec_length + initiator.EXPANSION);
        }

        // Decrypt
        std::vector<std::byte> decrypt(dec_length);
        bool dec_ignore{false};
        bool ok = receiver.Decrypt(std::span{ciphertext}.subspan(initiator.LENGTH_LEN), aad, dec_ignore, decrypt);
        // Decryption *must* fail if the packet was damaged, and succeed if it wasn't.
        assert(!ok == damage);
        if (!ok) break;
        assert(ignore == dec_ignore);
        assert(decrypt == contents);
    }

    // Force both directions through the exact rekey boundary. Random input may stop before 224
    // packets, while a sender/receiver rekey mismatch only becomes observable on the next packet.
    AssertRekeyBoundary(init_key, init_ent, resp_key, resp_ent);
}
