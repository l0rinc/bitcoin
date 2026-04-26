// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hkdf_sha256_32.h>

#include <crypto/hmac_sha256.h>

#include <cassert>

CHKDF_HMAC_SHA256_L32::CHKDF_HMAC_SHA256_L32(std::span<const unsigned char> ikm, std::span<const unsigned char> salt)
{
    CHMAC_SHA256(salt.data(), salt.size()).Write(ikm.data(), ikm.size()).Finalize(m_prk);
}

void CHKDF_HMAC_SHA256_L32::Expand32(std::span<const unsigned char> info, std::span<unsigned char> hash)
{
    // expand a 32byte key (single round)
    assert(info.size() <= 128);
    assert(hash.size() == OUTPUT_SIZE);
    static const unsigned char one[1] = {1};
    CHMAC_SHA256(m_prk, 32).Write(info.data(), info.size()).Write(one, 1).Finalize(hash.data());
}
