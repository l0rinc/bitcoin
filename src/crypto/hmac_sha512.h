// Copyright (c) 2014-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_HMAC_SHA512_H
#define BITCOIN_CRYPTO_HMAC_SHA512_H

#include <crypto/sha512.h>
#include <span.h>

#include <cassert>
#include <cstddef>

/** A hasher class for HMAC-SHA-512. */
class CHMAC_SHA512
{
private:
    CSHA512 outer;
    CSHA512 inner;

public:
    static const size_t OUTPUT_SIZE = 64;

    CHMAC_SHA512(const unsigned char* key, size_t keylen);
    explicit CHMAC_SHA512(std::span<const unsigned char> key) : CHMAC_SHA512(key.data(), key.size()) {}
    CHMAC_SHA512& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    CHMAC_SHA512& Write(std::span<const unsigned char> data) { return Write(data.data(), data.size()); }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
    void Finalize(std::span<unsigned char> hash)
    {
        assert(hash.size() == OUTPUT_SIZE);
        Finalize(hash.data());
    }
};

#endif // BITCOIN_CRYPTO_HMAC_SHA512_H
