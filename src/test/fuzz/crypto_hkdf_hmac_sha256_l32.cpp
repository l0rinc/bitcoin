// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>
#include <span.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <cstdint>
#include <string>
#include <vector>

FUZZ_TARGET(crypto_hkdf_hmac_sha256_l32)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const std::vector<uint8_t> initial_key_material = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    const auto salt{fuzzed_data_provider.ConsumeRandomLengthString(1024)};
    CHKDF_HMAC_SHA256_L32 hkdf_hmac_sha256_l32(MakeUCharSpan(initial_key_material), MakeUCharSpan(salt));
    LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 10000) {
        const auto info{fuzzed_data_provider.ConsumeRandomLengthString(128)};
        std::vector<uint8_t> out(32);
        hkdf_hmac_sha256_l32.Expand32(MakeUCharSpan(info), UCharSpanCast(std::span{out}));
    }
}
