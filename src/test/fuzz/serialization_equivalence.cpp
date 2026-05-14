// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <prevector.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace {

std::vector<std::byte> ToBytes(const DataStream& stream)
{
    return {stream.begin(), stream.end()};
}

std::vector<std::byte> ConsumeBytes(FuzzedDataProvider& provider, size_t size)
{
    std::vector<std::byte> bytes(size);
    for (auto& byte : bytes) {
        byte = std::byte{provider.ConsumeIntegral<unsigned char>()};
    }
    return bytes;
}

template <size_t N>
std::array<std::byte, N> FixedInput(std::span<const std::byte> input)
{
    std::array<std::byte, N> out{};
    std::copy_n(input.begin(), N, out.begin());
    return out;
}

template <size_t N>
void CheckStreamSpanEquivalence(FuzzedDataProvider& provider)
{
    const auto input{ConsumeBytes(provider, provider.ConsumeIntegralInRange<size_t>(N, 1024))};

    const auto fixed_input{FixedInput<N>(input)};

    DataStream dynamic_write;
    dynamic_write.write(std::span<const std::byte>{fixed_input});
    DataStream fixed_write;
    fixed_write.write(std::span<const std::byte, N>{fixed_input});
    assert(ToBytes(dynamic_write) == ToBytes(fixed_write));

    auto mutable_fixed_input{fixed_input};
    DataStream mutable_fixed_write;
    mutable_fixed_write.write(std::span<std::byte, N>{mutable_fixed_input});
    assert(ToBytes(dynamic_write) == ToBytes(mutable_fixed_write));

    std::array<std::byte, N> fixed_out{};
    std::vector<std::byte> dynamic_out(N);
    DataStream dynamic_read{input};
    DataStream fixed_read{input};
    dynamic_read.read(std::span<std::byte>{dynamic_out});
    fixed_read.read(std::span<std::byte, N>{fixed_out});
    assert(std::equal(fixed_out.begin(), fixed_out.end(), dynamic_out.begin(), dynamic_out.end()));
    assert(dynamic_read.size() == fixed_read.size());

    std::fill(dynamic_out.begin(), dynamic_out.end(), std::byte{0});
    fixed_out = {};
    SpanReader dynamic_span_read{input};
    SpanReader fixed_span_read{input};
    dynamic_span_read.read(std::span<std::byte>{dynamic_out});
    fixed_span_read.read(std::span<std::byte, N>{fixed_out});
    assert(std::equal(fixed_out.begin(), fixed_out.end(), dynamic_out.begin(), dynamic_out.end()));
    assert(dynamic_span_read.size() == fixed_span_read.size());
}

void CheckStreamSpanEquivalence(FuzzedDataProvider& provider)
{
    switch (provider.ConsumeIntegralInRange<int>(0, 3)) {
    case 0:
        CheckStreamSpanEquivalence<1>(provider);
        break;
    case 1:
        CheckStreamSpanEquivalence<4>(provider);
        break;
    case 2:
        CheckStreamSpanEquivalence<8>(provider);
        break;
    case 3:
        CheckStreamSpanEquivalence<32>(provider);
        break;
    }
}

void CheckSizeComputerEquivalence(FuzzedDataProvider& provider, uint64_t value)
{
    const auto payload{ConsumeBytes(provider, provider.ConsumeIntegralInRange<size_t>(0, 1024))};
    std::vector<std::byte> vector_payload{payload.begin(), payload.end()};
    prevector<28, unsigned char> prevector_payload;
    prevector_payload.resize(payload.size());
    std::transform(payload.begin(), payload.end(), prevector_payload.begin(), [](std::byte b) {
        return std::to_integer<unsigned char>(b);
    });
    std::string string_payload{reinterpret_cast<const char*>(payload.data()), payload.size()};

    auto serialize = [&](auto& stream) {
        stream << static_cast<uint8_t>(value);
        stream << static_cast<uint32_t>(value);
        stream << static_cast<uint64_t>(value);
        stream << VARINT(value);
        WriteCompactSize(stream, payload.size());
        stream << std::span<const std::byte>{payload};
        stream << vector_payload;
        stream << prevector_payload;
        stream << string_payload;
    };

    DataStream bytes;
    SizeComputer size;
    serialize(bytes);
    serialize(size);
    assert(size.size() == bytes.size());
}

} // namespace

FUZZ_TARGET(serialization_equivalence)
{
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    const uint64_t value{provider.ConsumeIntegral<uint64_t>()};

    LIMITED_WHILE(provider.remaining_bytes(), 16)
    {
        if (provider.ConsumeBool()) {
            CheckStreamSpanEquivalence(provider);
        } else {
            CheckSizeComputerEquivalence(provider, value);
        }
    }
}
