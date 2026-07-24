// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bloom.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <uint256.h>

#include <cassert>
#include <limits>
#include <optional>
#include <vector>

namespace {

std::vector<std::byte> SerializeBloomFilter(const CBloomFilter& filter)
{
    DataStream stream;
    stream << filter;
    return {stream.begin(), stream.end()};
}

void AssertBloomFilterRoundTrip(const CBloomFilter& filter)
{
    const std::vector<std::byte> serialized{SerializeBloomFilter(filter)};
    DataStream stream{std::span<const std::byte>{serialized}};
    CBloomFilter decoded;
    stream >> decoded;
    assert(stream.empty());
    assert(SerializeBloomFilter(decoded) == serialized);
    assert(decoded.IsWithinSizeConstraints() == filter.IsWithinSizeConstraints());
}

void ExerciseRelevantAndUpdateContracts()
{
    const std::vector<unsigned char> pubkey{
        0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0,
        0x62, 0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d,
        0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98};

    CMutableTransaction mutable_transaction;
    mutable_transaction.vout.emplace_back(1, CScript{} << pubkey << OP_CHECKSIG);
    const CTransaction transaction{mutable_transaction};
    const COutPoint outpoint{transaction.GetHash(), 0};

    for (const unsigned char flags : {BLOOM_UPDATE_NONE, BLOOM_UPDATE_ALL, BLOOM_UPDATE_P2PUBKEY_ONLY}) {
        CBloomFilter filter{1000, 1e-9, 0x12345678, flags};
        filter.insert(pubkey);
        assert(filter.IsRelevantAndUpdate(transaction));
        if (flags != BLOOM_UPDATE_NONE) {
            assert(filter.contains(outpoint));
        }
        AssertBloomFilterRoundTrip(filter);
    }
}

} // namespace

FUZZ_TARGET(bloom_filter)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    bool good_data{true};
    ExerciseRelevantAndUpdateContracts();

    CBloomFilter bloom_filter{
        fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, 10000000),
        1.0 / fuzzed_data_provider.ConsumeIntegralInRange<unsigned int>(1, std::numeric_limits<unsigned int>::max()),
        fuzzed_data_provider.ConsumeIntegral<unsigned int>(),
        static_cast<unsigned char>(fuzzed_data_provider.PickValueInArray({BLOOM_UPDATE_NONE, BLOOM_UPDATE_ALL, BLOOM_UPDATE_P2PUBKEY_ONLY, BLOOM_UPDATE_MASK}))};
    assert(bloom_filter.IsWithinSizeConstraints());
    LIMITED_WHILE (good_data && fuzzed_data_provider.remaining_bytes() > 0, 10'000) {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                const std::vector<unsigned char> b = ConsumeRandomLengthByteVector(fuzzed_data_provider);
                (void)bloom_filter.contains(b);
                bloom_filter.insert(b);
                const bool present = bloom_filter.contains(b);
                assert(present);
            },
            [&] {
                const std::optional<COutPoint> out_point = ConsumeDeserializable<COutPoint>(fuzzed_data_provider);
                if (!out_point) {
                    good_data = false;
                    return;
                }
                (void)bloom_filter.contains(*out_point);
                bloom_filter.insert(*out_point);
                const bool present = bloom_filter.contains(*out_point);
                assert(present);
            },
            [&] {
                const std::optional<uint256> u256 = ConsumeDeserializable<uint256>(fuzzed_data_provider);
                if (!u256) {
                    good_data = false;
                    return;
                }
                (void)bloom_filter.contains(*u256);
                bloom_filter.insert(*u256);
                const bool present = bloom_filter.contains(*u256);
                assert(present);
            },
            [&] {
                const std::optional<CMutableTransaction> mut_tx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
                if (!mut_tx) {
                    good_data = false;
                    return;
                }
                const CTransaction tx{*mut_tx};
                (void)bloom_filter.IsRelevantAndUpdate(tx);
            });
        (void)bloom_filter.IsWithinSizeConstraints();
    }
    AssertBloomFilterRoundTrip(bloom_filter);
}
