// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

int ec_seckey_import_der(const secp256k1_context* ctx, unsigned char* out32, const unsigned char* seckey, size_t seckeylen);
int ec_seckey_export_der(const secp256k1_context* ctx, unsigned char* seckey, size_t* seckeylen, const unsigned char* key32, bool compressed);

FUZZ_TARGET(secp256k1_ec_seckey_import_export_der)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    {
        const std::vector<uint8_t> der = ConsumeFixedLengthByteVector(fuzzed_data_provider, CKey::SIZE);
        std::array<uint8_t, 32> out32;
        out32.fill(0xa5);
        const bool imported = ec_seckey_import_der(secp256k1_context_static, out32.data(), der.data(), der.size()) == 1;
        if (!imported) {
            for (const uint8_t byte : out32) {
                assert(byte == 0);
            }
        }
    }
    {
        std::vector<uint8_t> seckey(CKey::SIZE);
        const std::vector<uint8_t> key32 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
        size_t seckeylen = CKey::SIZE;
        const bool compressed = fuzzed_data_provider.ConsumeBool();
        secp256k1_context* secp256k1_context_sign = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        const bool exported = ec_seckey_export_der(secp256k1_context_sign, seckey.data(), &seckeylen, key32.data(), compressed);
        secp256k1_context_destroy(secp256k1_context_sign);
        if (exported) {
            assert(seckeylen == (compressed ? CKey::COMPRESSED_SIZE : CKey::SIZE));
            std::array<uint8_t, 32> out32;
            const bool imported = ec_seckey_import_der(secp256k1_context_static, out32.data(), seckey.data(), seckey.size()) == 1;
            assert(imported && std::equal(key32.begin(), key32.end(), out32.begin()));
        } else {
            assert(seckeylen == 0);
        }
    }

    // Wallet loading can pass data() from an empty CPrivKey. The import contract is a clean
    // rejection with a cleared output, even when the input pointer is null and the length is zero.
    std::array<uint8_t, 32> empty_out;
    empty_out.fill(0xa5);
    const bool empty_imported = ec_seckey_import_der(secp256k1_context_static, empty_out.data(), nullptr, 0) == 1;
    assert(!empty_imported);
    for (const uint8_t byte : empty_out) {
        assert(byte == 0);
    }
}
