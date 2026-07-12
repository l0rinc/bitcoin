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
#include <cassert>
#include <cstdint>
#include <vector>

bool SigHasLowR(const secp256k1_ecdsa_signature* sig);
int ecdsa_signature_parse_der_lax(secp256k1_ecdsa_signature* sig, const unsigned char* input, size_t inputlen);

namespace {
std::array<unsigned char, 64> SerializeCompact(const secp256k1_ecdsa_signature& sig)
{
    std::array<unsigned char, 64> compact_sig{};
    assert(secp256k1_ecdsa_signature_serialize_compact(secp256k1_context_static, compact_sig.data(), &sig));
    return compact_sig;
}

bool IsZeroCompact(const std::array<unsigned char, 64>& compact_sig)
{
    return std::all_of(compact_sig.begin(), compact_sig.end(), [](unsigned char byte) { return byte == 0; });
}
} // namespace

FUZZ_TARGET(secp256k1_ecdsa_signature_parse_der_lax)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const std::vector<uint8_t> signature_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    const uint8_t empty_data{0};
    const unsigned char* signature_data{signature_bytes.empty() ? &empty_data : signature_bytes.data()};

    secp256k1_ecdsa_signature sig_der_lax;
    const bool parsed_der_lax{
        ecdsa_signature_parse_der_lax(&sig_der_lax, signature_data, signature_bytes.size()) == 1};

    secp256k1_ecdsa_signature sig_der_strict;
    const int parsed_der_strict_raw{secp256k1_ecdsa_signature_parse_der(
        secp256k1_context_static,
        &sig_der_strict,
        signature_data,
        signature_bytes.size())};
    const bool parsed_der_strict{parsed_der_strict_raw == 1};
    assert(!parsed_der_strict || parsed_der_lax);

    if (!parsed_der_lax) {
        assert(IsZeroCompact(SerializeCompact(sig_der_lax)));
        return;
    }

    ECC_Context ecc_context{};
    const std::array<unsigned char, 64> compact_der_lax{SerializeCompact(sig_der_lax)};
    assert(SigHasLowR(&sig_der_lax) == (compact_der_lax[0] < 0x80));

    secp256k1_ecdsa_signature sig_compact_roundtrip;
    if (secp256k1_ecdsa_signature_parse_compact(
            secp256k1_context_static, &sig_compact_roundtrip, compact_der_lax.data()) == 1) {
        assert(SerializeCompact(sig_compact_roundtrip) == compact_der_lax);
    }

    if (parsed_der_strict) {
        const std::array<unsigned char, 64> compact_der_strict{SerializeCompact(sig_der_strict)};
        secp256k1_ecdsa_signature sig_strict_compact_roundtrip;
        const bool strict_r_nonzero{std::ranges::any_of(compact_der_strict.begin(), compact_der_strict.begin() + 32,
                                                        [](unsigned char byte) { return byte != 0; })};
        const bool strict_s_nonzero{std::ranges::any_of(compact_der_strict.begin() + 32, compact_der_strict.end(),
                                                        [](unsigned char byte) { return byte != 0; })};
        if (strict_r_nonzero && strict_s_nonzero && secp256k1_ecdsa_signature_parse_compact(
                secp256k1_context_static, &sig_strict_compact_roundtrip, compact_der_strict.data()) == 1) {
            assert(compact_der_strict == compact_der_lax);
        }
    }
}
