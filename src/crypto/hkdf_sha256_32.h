// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HKDF_SHA256_32_H
#define BITCOIN_CRYPTO_HKDF_SHA256_32_H

#include <support/cleanse.h>

#include <cstddef>
#include <string>

/** A rfc5869 HKDF implementation with HMAC_SHA256 and fixed key output length of 32 bytes (L=32) */
class CHKDF_HMAC_SHA256_L32
{
private:
    unsigned char m_prk[32];
    static constexpr size_t OUTPUT_SIZE{32};

public:
    // Avoid duplicating the pseudorandom key held by this context.
    CHKDF_HMAC_SHA256_L32(const CHKDF_HMAC_SHA256_L32&) = delete;
    CHKDF_HMAC_SHA256_L32& operator=(const CHKDF_HMAC_SHA256_L32&) = delete;
    CHKDF_HMAC_SHA256_L32(CHKDF_HMAC_SHA256_L32&&) = delete;
    CHKDF_HMAC_SHA256_L32& operator=(CHKDF_HMAC_SHA256_L32&&) = delete;

    CHKDF_HMAC_SHA256_L32(const unsigned char* ikm, size_t ikmlen, const std::string& salt);
    ~CHKDF_HMAC_SHA256_L32() { memory_cleanse(m_prk, sizeof(m_prk)); }
    void Expand32(const std::string& info, unsigned char hash[OUTPUT_SIZE]);
};

#endif // BITCOIN_CRYPTO_HKDF_SHA256_32_H
