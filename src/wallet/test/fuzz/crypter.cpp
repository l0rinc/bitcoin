// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/crypter.h>

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <array>
#include <string>

namespace wallet {
namespace {

void initialize_crypter()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
}

void AssertCrypterContracts()
{
    const CKeyingMaterial master_key(WALLET_CRYPTO_KEY_SIZE, 0x42);
    const CKeyingMaterial plaintext(WALLET_CRYPTO_KEY_SIZE, 0x24);
    constexpr std::array<unsigned char, WALLET_CRYPTO_IV_SIZE> zero_iv{};
    const std::vector<unsigned char> expected_ciphertext{ParseHex(std::string{
        "f3bd0549faac94a41a5e14396d29b64a"
        "e44710ebd1725d705c140feaf1269c98"
        "156c9c65adf90de74cb5ba58c73c51df"})};

    std::vector<unsigned char> ciphertext;
    assert(EncryptSecret(master_key, plaintext, uint256{}, ciphertext));
    assert(ciphertext == expected_ciphertext);

    CKeyingMaterial decrypted;
    assert(DecryptSecret(master_key, ciphertext, uint256{}, decrypted));
    assert(decrypted == plaintext);

    std::vector<unsigned char> damaged_ciphertext{ciphertext};
    damaged_ciphertext.back() = 0;
    assert(!DecryptSecret(master_key, damaged_ciphertext, uint256{}, decrypted));

    CCrypter crypt;
    assert(!crypt.Encrypt(plaintext, ciphertext));
    assert(crypt.SetKey(master_key, zero_iv));
    assert(crypt.Encrypt(plaintext, ciphertext));
    assert(ciphertext == expected_ciphertext);
    assert(crypt.Decrypt(ciphertext, decrypted));
    assert(decrypted == plaintext);
    crypt.CleanKey();
    assert(!crypt.Encrypt(plaintext, ciphertext));

    std::array<unsigned char, WALLET_CRYPTO_SALT_SIZE - 1> short_salt{};
    CCrypter invalid;
    assert(!invalid.SetKeyFromPassphrase("test", short_salt, 1, 0));
    assert(!invalid.Encrypt(plaintext, ciphertext));

    // An unsupported derivation method must invalidate a previously usable key. The production
    // method cleanses the key bytes on this path, so retaining fKeySet would make Encrypt report
    // success with a zeroized key and IV.
    CCrypter failed_rekey;
    assert(failed_rekey.SetKey(master_key, zero_iv));
    const std::array<unsigned char, WALLET_CRYPTO_SALT_SIZE> valid_salt{};
    assert(!failed_rekey.SetKeyFromPassphrase("test", valid_salt, 1, /*unsupported=*/1));
    assert(!failed_rekey.Encrypt(plaintext, ciphertext));

    std::array<unsigned char, WALLET_CRYPTO_KEY_SIZE> secret_bytes{};
    secret_bytes.fill(1);
    CKey source_key;
    source_key.Set(secret_bytes.begin(), secret_bytes.end(), /*fCompressedIn=*/true);
    assert(source_key.IsValid());
    const CPubKey pubkey{source_key.GetPubKey()};
    const CKeyingMaterial secret{secret_bytes.begin(), secret_bytes.end()};
    assert(EncryptSecret(master_key, secret, pubkey.GetHash(), ciphertext));

    CKey recovered_key;
    assert(DecryptKey(master_key, ciphertext, pubkey, recovered_key));
    assert(recovered_key.IsValid());
    assert(recovered_key.GetPubKey() == pubkey);
}

FUZZ_TARGET(crypter, .init = initialize_crypter)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    AssertCrypterContracts();
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    bool good_data{true};

    CCrypter crypt;
    // These values are regularly updated within `CallOneOf`
    std::vector<unsigned char> cipher_text_ed;
    CKeyingMaterial plain_text_ed;
    const std::vector<unsigned char> random_key = ConsumeFixedLengthByteVector(fuzzed_data_provider, WALLET_CRYPTO_KEY_SIZE);

    if (fuzzed_data_provider.ConsumeBool()) {
        const std::string random_string = fuzzed_data_provider.ConsumeRandomLengthString(100);
        SecureString secure_string(random_string.begin(), random_string.end());

        const unsigned int derivation_method = fuzzed_data_provider.ConsumeBool() ? 0 : fuzzed_data_provider.ConsumeIntegral<unsigned int>();

        // Limiting the value of rounds since it is otherwise uselessly expensive and causes a timeout when fuzzing.
        crypt.SetKeyFromPassphrase(/*key_data=*/secure_string,
                                   /*salt=*/ConsumeFixedLengthByteVector(fuzzed_data_provider, WALLET_CRYPTO_SALT_SIZE),
                                   /*rounds=*/fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(0, CMasterKey::DEFAULT_DERIVE_ITERATIONS),
                                   /*derivation_method=*/derivation_method);
    }

    CKey random_ckey;
    random_ckey.Set(random_key.begin(), random_key.end(), /*fCompressedIn=*/fuzzed_data_provider.ConsumeBool());
    if (!random_ckey.IsValid()) return;
    CPubKey pubkey{random_ckey.GetPubKey()};

    LIMITED_WHILE (good_data && fuzzed_data_provider.ConsumeBool(), 100) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> random_vector = ConsumeFixedLengthByteVector(fuzzed_data_provider, WALLET_CRYPTO_KEY_SIZE);
                plain_text_ed = CKeyingMaterial(random_vector.begin(), random_vector.end());
            },
            [&] {
                cipher_text_ed = ConsumeRandomLengthByteVector(fuzzed_data_provider, 64);
            },
            [&] {
                (void)crypt.Encrypt(plain_text_ed, cipher_text_ed);
            },
            [&] {
                (void)crypt.Decrypt(cipher_text_ed, plain_text_ed);
            },
            [&] {
                const CKeyingMaterial master_key(random_key.begin(), random_key.end());
                (void)EncryptSecret(master_key, plain_text_ed, pubkey.GetHash(), cipher_text_ed);
            },
            [&] {
                std::optional<CPubKey> random_pub_key{ConsumeDeserializable<CPubKey>(fuzzed_data_provider)};
                if (!random_pub_key) {
                    good_data = false;
                    return;
                }
                pubkey = *random_pub_key;
            },
            [&] {
                const CKeyingMaterial master_key(random_key.begin(), random_key.end());
                CKey key;
                (void)DecryptKey(master_key, cipher_text_ed, pubkey, key);
            });
    }
}
} // namespace
} // namespace wallet
