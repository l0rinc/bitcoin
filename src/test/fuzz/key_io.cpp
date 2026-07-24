// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <key_io.h>
#include <test/fuzz/fuzz.h>
#include <util/chaintype.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

void initialize_key_io()
{
    static ECC_Context ecc_context{};
    SelectParams(ChainType::MAIN);
}

FUZZ_TARGET(key_io, .init = initialize_key_io)
{
    const std::string random_string(buffer.begin(), buffer.end());

    const CKey key = DecodeSecret(random_string);
    if (key.IsValid()) {
        assert(key == DecodeSecret(EncodeSecret(key)));
    }

    std::array<unsigned char, 32> generated_secret{};
    for (size_t i = 0; i < generated_secret.size() && i < buffer.size(); ++i) {
        generated_secret[i] = buffer[i];
    }
    const bool compressed = buffer.size() > generated_secret.size() && (buffer[generated_secret.size()] & 1) != 0;
    CKey generated_key;
    generated_key.Set(generated_secret.data(), generated_secret.data() + generated_secret.size(), compressed);
    if (!generated_key.IsValid()) {
        generated_secret.fill(0);
        generated_secret.back() = 1;
        generated_key.Set(generated_secret.data(), generated_secret.data() + generated_secret.size(), compressed);
    }
    assert(generated_key.IsValid());
    assert(generated_key == DecodeSecret(EncodeSecret(generated_key)));

    const CExtKey ext_key = DecodeExtKey(random_string);
    if (ext_key.key.size() == 32) {
        assert(ext_key == DecodeExtKey(EncodeExtKey(ext_key)));
    }

    const CExtPubKey ext_pub_key = DecodeExtPubKey(random_string);
    if (ext_pub_key.pubkey.size() == CPubKey::COMPRESSED_SIZE) {
        assert(ext_pub_key == DecodeExtPubKey(EncodeExtPubKey(ext_pub_key)));
    }
}
