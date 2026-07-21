// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <secp256k1.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

bool SigHasLowR(const secp256k1_ecdsa_signature* sig);
int ecdsa_signature_parse_der_lax(secp256k1_ecdsa_signature* sig, const unsigned char* input, size_t inputlen);

FUZZ_TARGET(secp256k1_ecdsa_signature_parse_der_lax)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> signature_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);

    const auto serialize_compact = [](const secp256k1_ecdsa_signature* sig) {
        std::array<uint8_t, 64> compact;
        secp256k1_ecdsa_signature_serialize_compact(secp256k1_context_static, compact.data(), sig);
        return compact;
    };
    const auto is_zero = [](const std::array<uint8_t, 64>& compact) {
        return std::all_of(compact.begin(), compact.end(), [](const uint8_t byte) { return byte == 0; });
    };

    std::array<uint8_t, 64> initial_compact;
    initial_compact.fill(1);
    secp256k1_ecdsa_signature sig_der_lax;
    assert(secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &sig_der_lax, initial_compact.data()) == 1);
    const bool parsed_der_lax = ecdsa_signature_parse_der_lax(&sig_der_lax, signature_bytes.data(), signature_bytes.size()) == 1;
    const std::array<uint8_t, 64> compact_lax = serialize_compact(&sig_der_lax);
    if (!parsed_der_lax) {
        assert(is_zero(compact_lax));
    } else {
        ECC_Context ecc_context{};
        const bool low_r = SigHasLowR(&sig_der_lax);
        assert(low_r == SigHasLowR(&sig_der_lax));
        assert(low_r == (compact_lax[0] < 0x80));

        const bool nonzero_r = std::any_of(compact_lax.begin(), compact_lax.begin() + 32, [](const uint8_t byte) { return byte != 0; });
        const bool nonzero_s = std::any_of(compact_lax.begin() + 32, compact_lax.end(), [](const uint8_t byte) { return byte != 0; });
        if (nonzero_r && nonzero_s) {
            std::array<uint8_t, CPubKey::SIGNATURE_SIZE> der_roundtrip;
            size_t der_roundtrip_len = der_roundtrip.size();
            assert(secp256k1_ecdsa_signature_serialize_der(secp256k1_context_static, der_roundtrip.data(), &der_roundtrip_len, &sig_der_lax) == 1);
            secp256k1_ecdsa_signature sig_roundtrip;
            assert(ecdsa_signature_parse_der_lax(&sig_roundtrip, der_roundtrip.data(), der_roundtrip_len) == 1);
            assert(compact_lax == serialize_compact(&sig_roundtrip));
        }

        // Strict DER is a subset of the historical lax parser's accepted domain.
        if (!signature_bytes.empty()) {
            secp256k1_ecdsa_signature sig_der_strict;
            const bool parsed_der_strict = secp256k1_ecdsa_signature_parse_der(secp256k1_context_static, &sig_der_strict, signature_bytes.data(), signature_bytes.size()) == 1;
            if (parsed_der_strict) {
                assert(compact_lax == serialize_compact(&sig_der_strict));
            }
        }
    }

    // The production parser accepts a null pointer for the empty-input case and must
    // still return its canonical invalid-signature state.
    secp256k1_ecdsa_signature empty_sig;
    assert(secp256k1_ecdsa_signature_parse_compact(secp256k1_context_static, &empty_sig, initial_compact.data()) == 1);
    assert(ecdsa_signature_parse_der_lax(&empty_sig, nullptr, 0) == 0);
    assert(is_zero(serialize_compact(&empty_sig)));
}
