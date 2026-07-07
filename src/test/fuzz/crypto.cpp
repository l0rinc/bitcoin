// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hmac_sha256.h>
#include <crypto/hmac_sha512.h>
#include <crypto/ripemd160.h>
#include <crypto/sha1.h>
#include <crypto/sha256.h>
#include <crypto/sha3.h>
#include <crypto/sha512.h>
#include <hash.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

namespace {
void CheckHashWrapperChunking(const std::vector<uint8_t>& data, const size_t split_pos)
{
    const auto input{std::span{data}};
    const auto split{std::min(split_pos, input.size())};
    const auto first{input.first(split)};
    const auto second{input.subspan(split)};

    uint256 hash256_split;
    CHash256().Write(first).Write(second).Finalize(hash256_split);
    assert(hash256_split == Hash(data));
    assert(Hash(first, second) == Hash(data));

    uint160 hash160_split;
    CHash160().Write(first).Write(second).Finalize(hash160_split);
    assert(hash160_split == Hash160(data));
}

template <typename Hasher>
std::vector<uint8_t> FinalizeHasher(Hasher hasher)
{
    std::vector<uint8_t> out(Hasher::OUTPUT_SIZE);
    hasher.Finalize(out.data());
    return out;
}

template <typename Hasher>
void CheckHasherChunking(const std::vector<uint8_t>& data, const size_t split_pos)
{
    const auto input{std::span{data}};
    const auto split{std::min(split_pos, input.size())};

    Hasher whole;
    whole.Write(input.data(), input.size());
    const std::vector<uint8_t> whole_out{FinalizeHasher(whole)};

    Hasher chunked;
    chunked.Write(input.data(), split);
    Hasher copied{chunked};
    chunked.Write(input.subspan(split).data(), input.size() - split);
    copied.Write(input.subspan(split).data(), input.size() - split);
    assert(FinalizeHasher(chunked) == whole_out);
    assert(FinalizeHasher(copied) == whole_out);
}

template <typename Hasher>
void CheckHmacChunking(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data, const size_t split_pos)
{
    const auto input{std::span{data}};
    const auto split{std::min(split_pos, input.size())};

    Hasher whole{key.data(), key.size()};
    whole.Write(input.data(), input.size());
    const std::vector<uint8_t> whole_out{FinalizeHasher(whole)};

    Hasher chunked{key.data(), key.size()};
    chunked.Write(input.data(), split);
    Hasher copied{chunked};
    chunked.Write(input.subspan(split).data(), input.size() - split);
    copied.Write(input.subspan(split).data(), input.size() - split);
    assert(FinalizeHasher(chunked) == whole_out);
    assert(FinalizeHasher(copied) == whole_out);
}

void CheckSHA3Chunking(const std::vector<uint8_t>& data, const size_t split_pos)
{
    const auto input{std::span{data}};
    const auto split{std::min(split_pos, input.size())};

    std::vector<uint8_t> whole_out(SHA3_256::OUTPUT_SIZE);
    SHA3_256{}.Write(input).Finalize(whole_out);

    std::vector<uint8_t> split_out(SHA3_256::OUTPUT_SIZE);
    SHA3_256{}.Write(input.first(split)).Write(input.subspan(split)).Finalize(split_out);
    assert(split_out == whole_out);
}

void CheckCryptoHasherChunking(const std::vector<uint8_t>& data, const size_t split_pos)
{
    CheckHasherChunking<CRIPEMD160>(data, split_pos);
    CheckHasherChunking<CSHA1>(data, split_pos);
    CheckHasherChunking<CSHA256>(data, split_pos);
    CheckHasherChunking<CSHA512>(data, split_pos);
    CheckHmacChunking<CHMAC_SHA256>(data, data, split_pos);
    CheckHmacChunking<CHMAC_SHA512>(data, data, split_pos);
    CheckSHA3Chunking(data, split_pos);
}
} // namespace

FUZZ_TARGET(crypto)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    std::vector<uint8_t> data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
    if (data.empty()) {
        auto new_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096);
        auto x = fuzzed_data_provider.ConsumeIntegral<uint8_t>();
        data.resize(new_size, x);
    }
    size_t split_pos{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, data.size())};
    CheckHashWrapperChunking(data, split_pos);
    CheckCryptoHasherChunking(data, split_pos);

    CHash160 hash160;
    CHash256 hash256;
    CHMAC_SHA256 hmac_sha256{data.data(), data.size()};
    CHMAC_SHA512 hmac_sha512{data.data(), data.size()};
    CRIPEMD160 ripemd160;
    CSHA1 sha1;
    CSHA256 sha256;
    CSHA512 sha512;
    SHA3_256 sha3;
    CSipHasher sip_hasher{fuzzed_data_provider.ConsumeIntegral<uint64_t>(), fuzzed_data_provider.ConsumeIntegral<uint64_t>()};

    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 30) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                if (fuzzed_data_provider.ConsumeBool()) {
                    data = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                    if (data.empty()) {
                        auto new_size = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 4096);
                        auto x = fuzzed_data_provider.ConsumeIntegral<uint8_t>();
                        data.resize(new_size, x);
                    }
                    split_pos = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, data.size());
                    CheckHashWrapperChunking(data, split_pos);
                    CheckCryptoHasherChunking(data, split_pos);
                }

                (void)hash160.Write(data);
                (void)hash256.Write(data);
                (void)hmac_sha256.Write(data.data(), data.size());
                (void)hmac_sha512.Write(data.data(), data.size());
                (void)ripemd160.Write(data.data(), data.size());
                (void)sha1.Write(data.data(), data.size());
                (void)sha256.Write(data.data(), data.size());
                (void)sha3.Write(data);
                (void)sha512.Write(data.data(), data.size());
                (void)sip_hasher.Write(data);

                (void)Hash(data);
                (void)Hash160(data);
                (void)sha512.Size();
            },
            [&] {
                (void)hash160.Reset();
                (void)hash256.Reset();
                (void)ripemd160.Reset();
                (void)sha1.Reset();
                (void)sha256.Reset();
                (void)sha3.Reset();
                (void)sha512.Reset();
            },
            [&] {
                CallOneOf(
                    fuzzed_data_provider,
                    [&] {
                        data.resize(CHash160::OUTPUT_SIZE);
                        hash160.Finalize(data);
                    },
                    [&] {
                        data.resize(CHash256::OUTPUT_SIZE);
                        hash256.Finalize(data);
                    },
                    [&] {
                        data.resize(CHMAC_SHA256::OUTPUT_SIZE);
                        hmac_sha256.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CHMAC_SHA512::OUTPUT_SIZE);
                        hmac_sha512.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CRIPEMD160::OUTPUT_SIZE);
                        ripemd160.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA1::OUTPUT_SIZE);
                        sha1.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA256::OUTPUT_SIZE);
                        sha256.Finalize(data.data());
                    },
                    [&] {
                        data.resize(CSHA512::OUTPUT_SIZE);
                        sha512.Finalize(data.data());
                    },
                    [&] {
                        data.resize(1);
                        data[0] = sip_hasher.Finalize() % 256;
                    },
                    [&] {
                        data.resize(SHA3_256::OUTPUT_SIZE);
                        sha3.Finalize(data);
                    });
            });
    }
    if (fuzzed_data_provider.ConsumeBool()) {
        uint64_t state[25];
        for (size_t i = 0; i < 25; ++i) {
            state[i] = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
        }
        KeccakF(state);
    }
}
