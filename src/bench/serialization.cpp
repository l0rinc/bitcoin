// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <bench/bench.h>
#include <prevector.h>
#include <serialize.h>
#include <streams.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

using SizeWeight = std::pair<size_t, size_t>;

template <size_t N>
size_t WeightedSize(const std::array<SizeWeight, N>& weights, size_t index)
{
    size_t total{0};
    for (const auto& [_, weight] : weights) total += weight;
    index %= total;
    for (const auto& [size, weight] : weights) {
        if (index < weight) return size;
        index -= weight;
    }
    assert(false);
    return 0;
}

template <size_t N>
std::array<std::byte, N> FixedBytes(size_t seed)
{
    std::array<std::byte, N> bytes{};
    for (size_t i{0}; i < bytes.size(); ++i) {
        bytes[i] = std::byte{static_cast<unsigned char>(seed + i)};
    }
    return bytes;
}

std::vector<std::byte> ByteVector(size_t size, size_t seed)
{
    std::vector<std::byte> bytes(size);
    for (size_t i{0}; i < bytes.size(); ++i) {
        bytes[i] = std::byte{static_cast<unsigned char>(seed + i)};
    }
    return bytes;
}

prevector<28, unsigned char> BytePrevector(size_t size, size_t seed)
{
    prevector<28, unsigned char> bytes;
    bytes.resize(size);
    for (size_t i{0}; i < bytes.size(); ++i) {
        bytes[i] = static_cast<unsigned char>(seed + i);
    }
    return bytes;
}

std::string StringOfSize(size_t size, size_t seed)
{
    std::string str(size, '\0');
    for (size_t i{0}; i < str.size(); ++i) {
        str[i] = static_cast<char>('a' + ((seed + i) % 26));
    }
    return str;
}

uint64_t CompactSizeValue(size_t index)
{
    constexpr std::array values{
        SizeWeight{22, 50},
        SizeWeight{23, 20},
        SizeWeight{106, 20},
        SizeWeight{162, 9},
        SizeWeight{253, 1},
    };
    return WeightedSize(values, index);
}

uint64_t VarIntValue(size_t index)
{
    constexpr std::array values{
        SizeWeight{42, 54},
        SizeWeight{300, 14},
        SizeWeight{70'000, 28},
        SizeWeight{3'000'000, 4},
    };
    return WeightedSize(values, index);
}

struct SerializationDistributionData {
    std::vector<std::array<std::byte, 1>> fixed_1;
    std::vector<std::array<std::byte, 4>> fixed_4;
    std::vector<std::array<std::byte, 8>> fixed_8;
    std::vector<std::array<std::byte, 32>> fixed_32;
    std::vector<std::vector<std::byte>> raw_spans;
    std::vector<std::vector<std::byte>> byte_vectors;
    std::vector<prevector<28, unsigned char>> byte_prevectors;
    std::vector<std::string> strings;
    std::vector<std::byte> serialized;
};

SerializationDistributionData CreateSerializationDistributionData()
{
    constexpr size_t samples{512};
    constexpr std::array raw_span_sizes{
        SizeWeight{1, 77},
        SizeWeight{32, 15},
        SizeWeight{21, 3},
        SizeWeight{22, 2},
        SizeWeight{34, 2},
        SizeWeight{105, 1},
    };
    constexpr std::array vector_sizes{
        SizeWeight{33, 37},
        SizeWeight{71, 29},
        SizeWeight{72, 15},
        SizeWeight{64, 7},
        SizeWeight{0, 5},
        SizeWeight{105, 3},
        SizeWeight{65, 1},
        SizeWeight{37, 1},
        SizeWeight{109, 1},
        SizeWeight{128, 1},
    };
    constexpr std::array prevector_sizes{
        SizeWeight{0, 20},
        SizeWeight{23, 19},
        SizeWeight{25, 18},
        SizeWeight{22, 14},
        SizeWeight{106, 8},
        SizeWeight{107, 7},
        SizeWeight{34, 6},
        SizeWeight{35, 2},
        SizeWeight{139, 1},
        SizeWeight{253, 1},
        SizeWeight{21, 1},
        SizeWeight{10, 1},
    };

    SerializationDistributionData data;
    data.fixed_1.reserve(samples);
    data.fixed_4.reserve(samples);
    data.fixed_8.reserve(samples);
    data.fixed_32.reserve(samples);
    data.raw_spans.reserve(samples);
    data.byte_vectors.reserve(samples);
    data.byte_prevectors.reserve(samples);
    data.strings.reserve(samples);

    for (size_t i{0}; i < samples; ++i) {
        data.fixed_1.push_back(FixedBytes<1>(i));
        data.fixed_4.push_back(FixedBytes<4>(i));
        data.fixed_8.push_back(FixedBytes<8>(i));
        data.fixed_32.push_back(FixedBytes<32>(i));
        data.raw_spans.push_back(ByteVector(WeightedSize(raw_span_sizes, i), i));
        data.byte_vectors.push_back(ByteVector(WeightedSize(vector_sizes, i), i));
        data.byte_prevectors.push_back(BytePrevector(WeightedSize(prevector_sizes, i), i));
        data.strings.push_back(StringOfSize(CompactSizeValue(i), i));
    }
    return data;
}

template <typename Stream>
void SerializeDistribution(Stream& stream, const SerializationDistributionData& data)
{
    for (size_t i{0}; i < data.byte_vectors.size(); ++i) {
        WriteCompactSize(stream, CompactSizeValue(i));
        WriteVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(stream, VarIntValue(i));
        stream << static_cast<uint8_t>(i);
        stream << static_cast<uint32_t>(i);
        stream << static_cast<uint64_t>(i);
        stream << std::span<const std::byte, 1>{data.fixed_1[i]};
        stream << std::span<const std::byte, 4>{data.fixed_4[i]};
        stream << std::span<const std::byte, 8>{data.fixed_8[i]};
        stream << std::span<const std::byte, 32>{data.fixed_32[i]};
        stream << std::span<const std::byte>{data.raw_spans[i]};
        stream << data.byte_vectors[i];
        stream << data.byte_prevectors[i];
        stream << data.strings[i];
    }
}

template <typename Stream>
void UnserializeDistribution(Stream& stream, const SerializationDistributionData& data)
{
    for (size_t i{0}; i < data.byte_vectors.size(); ++i) {
        uint8_t u8;
        uint32_t u32;
        uint64_t u64;
        std::array<std::byte, 1> fixed_1;
        std::array<std::byte, 4> fixed_4;
        std::array<std::byte, 8> fixed_8;
        std::array<std::byte, 32> fixed_32;
        std::vector<std::byte> raw_span(data.raw_spans[i].size());
        std::vector<std::byte> byte_vector;
        prevector<28, unsigned char> byte_prevector;
        std::string str;

        assert(ReadCompactSize(stream) == CompactSizeValue(i));
        assert((ReadVarInt<Stream, VarIntMode::DEFAULT, uint64_t>(stream)) == VarIntValue(i));
        stream >> u8 >> u32 >> u64;
        stream >> std::span<std::byte, 1>{fixed_1};
        stream >> std::span<std::byte, 4>{fixed_4};
        stream >> std::span<std::byte, 8>{fixed_8};
        stream >> std::span<std::byte, 32>{fixed_32};
        stream >> std::span<std::byte>{raw_span};
        stream >> byte_vector;
        stream >> byte_prevector;
        stream >> str;

        assert(u8 == static_cast<uint8_t>(i));
        assert(u32 == static_cast<uint32_t>(i));
        assert(u64 == static_cast<uint64_t>(i));
        assert(fixed_1 == data.fixed_1[i]);
        assert(fixed_4 == data.fixed_4[i]);
        assert(fixed_8 == data.fixed_8[i]);
        assert(fixed_32 == data.fixed_32[i]);
        assert(raw_span == data.raw_spans[i]);
        assert(byte_vector == data.byte_vectors[i]);
        assert(byte_prevector.size() == data.byte_prevectors[i].size());
        assert(std::equal(byte_prevector.begin(), byte_prevector.end(), data.byte_prevectors[i].begin()));
        assert(str == data.strings[i]);
    }
}

SerializationDistributionData CreateSerializedDistributionData()
{
    auto data{CreateSerializationDistributionData()};
    DataStream stream;
    SerializeDistribution(stream, data);
    data.serialized.assign(stream.begin(), stream.end());
    return data;
}

} // namespace

static void SizeComputerSerializationDistribution(benchmark::Bench& bench)
{
    const auto data{CreateSerializedDistributionData()};

    bench.batch(data.byte_vectors.size()).unit("item").run([&] {
        SizeComputer size_computer;
        SerializeDistribution(size_computer, data);
        assert(size_computer.size() == data.serialized.size());
    });
}

static void WriteSerializationDistribution(benchmark::Bench& bench)
{
    const auto data{CreateSerializedDistributionData()};

    bench.batch(data.byte_vectors.size()).unit("item").run([&] {
        DataStream stream;
        stream.reserve(data.serialized.size());
        SerializeDistribution(stream, data);
        assert(stream.size() == data.serialized.size());
    });
}

static void ReadSerializationDistribution(benchmark::Bench& bench)
{
    const auto data{CreateSerializedDistributionData()};

    bench.batch(data.byte_vectors.size()).unit("item").run([&] {
        SpanReader stream{data.serialized};
        UnserializeDistribution(stream, data);
        assert(stream.empty());
    });
}

BENCHMARK(SizeComputerSerializationDistribution);
BENCHMARK(WriteSerializationDistribution);
BENCHMARK(ReadSerializationDistribution);
