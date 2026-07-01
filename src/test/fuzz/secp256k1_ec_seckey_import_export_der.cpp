// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cstdint>
#include <vector>

int ec_seckey_import_der(const secp256k1_context* ctx, unsigned char* out32, const unsigned char* seckey, size_t seckeylen);
int ec_seckey_export_der(const secp256k1_context* ctx, unsigned char* seckey, size_t* seckeylen, const unsigned char* key32, bool compressed);

FUZZ_TARGET(secp256k1_ec_seckey_import_export_der)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    {
        std::vector<uint8_t> out32(32, 0xa5);
        const auto seckey{ConsumeFixedLengthByteVector(fuzzed_data_provider, CKey::SIZE)};
        const bool imported{ec_seckey_import_der(secp256k1_context_static, out32.data(), seckey.data(), seckey.size()) == 1};
        if (imported) {
            assert(secp256k1_ec_seckey_verify(secp256k1_context_static, out32.data()));
        } else {
            assert(std::ranges::all_of(out32, [](uint8_t byte) { return byte == 0; }));
        }
    }
    {
        std::vector<uint8_t> seckey(CKey::SIZE, 0xa5);
        const std::vector<uint8_t> key32 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 32);
        size_t seckeylen = CKey::SIZE;
        const bool compressed = fuzzed_data_provider.ConsumeBool();
        secp256k1_context* secp256k1_context_sign = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        const bool exported = ec_seckey_export_der(secp256k1_context_sign, seckey.data(), &seckeylen, key32.data(), compressed);
        secp256k1_context_destroy(secp256k1_context_sign);
        if (exported) {
            assert(seckeylen == (compressed ? CKey::COMPRESSED_SIZE : CKey::SIZE));
            assert(std::ranges::all_of(seckey.begin() + seckeylen, seckey.end(), [](uint8_t byte) { return byte == 0xa5; }));
            std::vector<uint8_t> out32(32);
            const bool imported = ec_seckey_import_der(secp256k1_context_static, out32.data(), seckey.data(), seckeylen) == 1;
            assert(imported && key32 == out32);
        } else {
            assert(seckeylen == 0);
        }
    }
}
