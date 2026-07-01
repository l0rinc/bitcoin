// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/common.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
template <size_t N>
std::array<uint8_t, N> ReversedBytes(const std::vector<uint8_t>& bytes)
{
    assert(bytes.size() == N);
    std::array<uint8_t, N> ret;
    std::copy(bytes.rbegin(), bytes.rend(), ret.begin());
    return ret;
}

template <size_t N>
std::array<std::byte, N> ToByteArray(const std::vector<uint8_t>& bytes)
{
    assert(bytes.size() == N);
    std::array<std::byte, N> ret;
    std::transform(bytes.begin(), bytes.end(), ret.begin(), [](uint8_t byte) { return std::byte{byte}; });
    return ret;
}

template <typename Array>
void AssertReversedBytesEqual(const Array& first, const Array& second)
{
    assert(std::equal(first.begin(), first.end(), second.rbegin()));
}

template <size_t N>
void AssertByteArraysEqual(const std::array<std::byte, N>& first, const std::array<uint8_t, N>& second)
{
    for (size_t i{0}; i < first.size(); ++i) {
        assert(std::to_integer<uint8_t>(first[i]) == second[i]);
    }
}
} // namespace

FUZZ_TARGET(crypto_common)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const uint16_t random_u16 = fuzzed_data_provider.ConsumeIntegral<uint16_t>();
    const uint32_t random_u32 = fuzzed_data_provider.ConsumeIntegral<uint32_t>();
    const uint64_t random_u64 = fuzzed_data_provider.ConsumeIntegral<uint64_t>();
    const std::vector<uint8_t> random_bytes_2 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 2);
    const std::vector<uint8_t> random_bytes_4 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 4);
    const std::vector<uint8_t> random_bytes_8 = ConsumeFixedLengthByteVector(fuzzed_data_provider, 8);
    const auto random_bytes_2_byte = ToByteArray<2>(random_bytes_2);
    const auto random_bytes_4_byte = ToByteArray<4>(random_bytes_4);
    const auto random_bytes_8_byte = ToByteArray<8>(random_bytes_8);

    std::array<uint8_t, 2> writele16_arr;
    WriteLE16(writele16_arr.data(), random_u16);
    assert(ReadLE16(writele16_arr.data()) == random_u16);
    std::array<std::byte, 2> writele16_byte_arr;
    WriteLE16(writele16_byte_arr.data(), random_u16);
    assert(ReadLE16(writele16_byte_arr.data()) == random_u16);
    AssertByteArraysEqual(writele16_byte_arr, writele16_arr);

    std::array<uint8_t, 4> writele32_arr;
    WriteLE32(writele32_arr.data(), random_u32);
    assert(ReadLE32(writele32_arr.data()) == random_u32);
    std::array<std::byte, 4> writele32_byte_arr;
    WriteLE32(writele32_byte_arr.data(), random_u32);
    assert(ReadLE32(writele32_byte_arr.data()) == random_u32);
    AssertByteArraysEqual(writele32_byte_arr, writele32_arr);

    std::array<uint8_t, 8> writele64_arr;
    WriteLE64(writele64_arr.data(), random_u64);
    assert(ReadLE64(writele64_arr.data()) == random_u64);
    std::array<std::byte, 8> writele64_byte_arr;
    WriteLE64(writele64_byte_arr.data(), random_u64);
    assert(ReadLE64(writele64_byte_arr.data()) == random_u64);
    AssertByteArraysEqual(writele64_byte_arr, writele64_arr);

    std::array<uint8_t, 2> writebe16_arr;
    WriteBE16(writebe16_arr.data(), random_u16);
    assert(ReadBE16(writebe16_arr.data()) == random_u16);
    AssertReversedBytesEqual(writele16_arr, writebe16_arr);
    std::array<std::byte, 2> writebe16_byte_arr;
    WriteBE16(writebe16_byte_arr.data(), random_u16);
    assert(ReadBE16(writebe16_byte_arr.data()) == random_u16);
    AssertByteArraysEqual(writebe16_byte_arr, writebe16_arr);
    AssertReversedBytesEqual(writele16_byte_arr, writebe16_byte_arr);

    std::array<uint8_t, 4> writebe32_arr;
    WriteBE32(writebe32_arr.data(), random_u32);
    assert(ReadBE32(writebe32_arr.data()) == random_u32);
    AssertReversedBytesEqual(writele32_arr, writebe32_arr);
    std::array<std::byte, 4> writebe32_byte_arr;
    WriteBE32(writebe32_byte_arr.data(), random_u32);
    assert(ReadBE32(writebe32_byte_arr.data()) == random_u32);
    AssertByteArraysEqual(writebe32_byte_arr, writebe32_arr);
    AssertReversedBytesEqual(writele32_byte_arr, writebe32_byte_arr);

    std::array<uint8_t, 8> writebe64_arr;
    WriteBE64(writebe64_arr.data(), random_u64);
    assert(ReadBE64(writebe64_arr.data()) == random_u64);
    AssertReversedBytesEqual(writele64_arr, writebe64_arr);
    std::array<std::byte, 8> writebe64_byte_arr;
    WriteBE64(writebe64_byte_arr.data(), random_u64);
    assert(ReadBE64(writebe64_byte_arr.data()) == random_u64);
    AssertByteArraysEqual(writebe64_byte_arr, writebe64_arr);
    AssertReversedBytesEqual(writele64_byte_arr, writebe64_byte_arr);

    const uint16_t readle16_result = ReadLE16(random_bytes_2.data());
    assert(ReadLE16(random_bytes_2_byte.data()) == readle16_result);
    std::array<uint8_t, 2> readle16_arr;
    WriteLE16(readle16_arr.data(), readle16_result);
    assert(std::memcmp(random_bytes_2.data(), readle16_arr.data(), 2) == 0);

    const uint32_t readle32_result = ReadLE32(random_bytes_4.data());
    assert(ReadLE32(random_bytes_4_byte.data()) == readle32_result);
    std::array<uint8_t, 4> readle32_arr;
    WriteLE32(readle32_arr.data(), readle32_result);
    assert(std::memcmp(random_bytes_4.data(), readle32_arr.data(), 4) == 0);

    const uint64_t readle64_result = ReadLE64(random_bytes_8.data());
    assert(ReadLE64(random_bytes_8_byte.data()) == readle64_result);
    std::array<uint8_t, 8> readle64_arr;
    WriteLE64(readle64_arr.data(), readle64_result);
    assert(std::memcmp(random_bytes_8.data(), readle64_arr.data(), 8) == 0);

    const uint16_t readbe16_result = ReadBE16(random_bytes_2.data());
    assert(ReadBE16(random_bytes_2_byte.data()) == readbe16_result);
    std::array<uint8_t, 2> readbe16_arr;
    WriteBE16(readbe16_arr.data(), readbe16_result);
    assert(std::memcmp(random_bytes_2.data(), readbe16_arr.data(), 2) == 0);
    assert(readbe16_result == ReadLE16(ReversedBytes<2>(random_bytes_2).data()));

    const uint32_t readbe32_result = ReadBE32(random_bytes_4.data());
    assert(ReadBE32(random_bytes_4_byte.data()) == readbe32_result);
    std::array<uint8_t, 4> readbe32_arr;
    WriteBE32(readbe32_arr.data(), readbe32_result);
    assert(std::memcmp(random_bytes_4.data(), readbe32_arr.data(), 4) == 0);
    assert(readbe32_result == ReadLE32(ReversedBytes<4>(random_bytes_4).data()));

    const uint64_t readbe64_result = ReadBE64(random_bytes_8.data());
    assert(ReadBE64(random_bytes_8_byte.data()) == readbe64_result);
    std::array<uint8_t, 8> readbe64_arr;
    WriteBE64(readbe64_arr.data(), readbe64_result);
    assert(std::memcmp(random_bytes_8.data(), readbe64_arr.data(), 8) == 0);
    assert(readbe64_result == ReadLE64(ReversedBytes<8>(random_bytes_8).data()));
}
