// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HMAC_SHA256_H
#define BITCOIN_CRYPTO_HMAC_SHA256_H

#include <crypto/sha256.h>
#include <span.h>

#include <cassert>
#include <cstddef>

/** A hasher class for HMAC-SHA-256. */
class CHMAC_SHA256
{
private:
    CSHA256 outer;
    CSHA256 inner;

public:
    static const size_t OUTPUT_SIZE = 32;

    CHMAC_SHA256(const unsigned char* key, size_t keylen);
    explicit CHMAC_SHA256(std::span<const unsigned char> key) : CHMAC_SHA256(key.data(), key.size()) {}
    CHMAC_SHA256& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    CHMAC_SHA256& Write(std::span<const unsigned char> data) { return Write(data.data(), data.size()); }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    void Finalize(std::span<unsigned char> hash)
    {
        assert(hash.size() == OUTPUT_SIZE);
        Finalize(hash.data());
    }
};

#endif // BITCOIN_CRYPTO_HMAC_SHA256_H
