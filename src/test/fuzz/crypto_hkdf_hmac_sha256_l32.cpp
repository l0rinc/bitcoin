// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(crypto_hkdf_hmac_sha256_l32)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> initial_key_material = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const std::string salt{fuzzed_data_provider.ConsumeRandomLengthString(1024)};

    CHKDF_HMAC_SHA256_L32 hkdf_hmac_sha256_l32(initial_key_material.data(), initial_key_material.size(), salt);
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000)
    {
        const std::string info{fuzzed_data_provider.ConsumeRandomLengthString(128)};
        std::array<unsigned char, 32> out;
        std::array<unsigned char, 32> repeated;
        std::array<unsigned char, 32> fresh;
        hkdf_hmac_sha256_l32.Expand32(info, out.data());
        hkdf_hmac_sha256_l32.Expand32(info, repeated.data());
        assert(out == repeated);

        CHKDF_HMAC_SHA256_L32 fresh_hkdf_hmac_sha256_l32(initial_key_material.data(), initial_key_material.size(), salt);
        fresh_hkdf_hmac_sha256_l32.Expand32(info, fresh.data());
        assert(out == fresh);
    }
}
