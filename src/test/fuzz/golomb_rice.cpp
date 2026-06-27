// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/random.h>
#include <util/bytevectorhash.h>
#include <util/check.h>
#include <util/golombrice.h>

#include <algorithm>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <unordered_set>
#include <vector>

namespace {

uint64_t HashToRange(const std::vector<uint8_t>& element, const uint64_t f)
{
    const uint64_t hash = CSipHasher(0x0706050403020100ULL, 0x0F0E0D0C0B0A0908ULL)
                              .Write(element)
                              .Finalize();
    return FastRange64(hash, f);
}

std::vector<uint64_t> BuildHashedSet(const std::unordered_set<std::vector<uint8_t>, ByteVectorHash>& elements, const uint64_t f)
{
    std::vector<uint64_t> hashed_elements;
    hashed_elements.reserve(elements.size());
    for (const std::vector<uint8_t>& element : elements) {
        hashed_elements.push_back(HashToRange(element, f));
    }
    std::sort(hashed_elements.begin(), hashed_elements.end());
    return hashed_elements;
}

void AssertGolombRiceRoundTrip(const std::vector<uint64_t>& values, const uint8_t p)
{
    std::vector<uint8_t> encoded;
    {
        VectorWriter stream{encoded, 0};
        BitStreamWriter bitwriter{stream};
        for (const uint64_t value : values) {
            GolombRiceEncode(bitwriter, p, value);
        }
        bitwriter.Flush();
    }

    SpanReader stream{encoded};
    BitStreamReader bitreader{stream};
    for (const uint64_t value : values) {
        Assert(GolombRiceDecode(bitreader, p) == value);
    }
    Assert(stream.empty());
}
} // namespace

FUZZ_TARGET(golomb_rice)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    std::vector<uint8_t> golomb_rice_data;
    std::vector<uint64_t> encoded_deltas;
    {
        std::unordered_set<std::vector<uint8_t>, ByteVectorHash> elements;
        const int n = fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 512);
        for (int i = 0; i < n; ++i) {
            elements.insert(ConsumeRandomLengthByteVector(fuzzed_data_provider, 16));
        }
        VectorWriter stream{golomb_rice_data, 0};
        WriteCompactSize(stream, static_cast<uint32_t>(elements.size()));
        BitStreamWriter bitwriter{stream};
        if (!elements.empty()) {
            uint64_t last_value = 0;
            for (const uint64_t value : BuildHashedSet(elements, static_cast<uint64_t>(elements.size()) * static_cast<uint64_t>(BASIC_FILTER_M))) {
                const uint64_t delta = value - last_value;
                encoded_deltas.push_back(delta);
                GolombRiceEncode(bitwriter, BASIC_FILTER_P, delta);
                last_value = value;
            }
        }
        bitwriter.Flush();
    }

    std::vector<uint64_t> decoded_deltas;
    {
        SpanReader stream{golomb_rice_data};
        BitStreamReader bitreader{stream};
        const uint32_t n = static_cast<uint32_t>(ReadCompactSize(stream));
        for (uint32_t i = 0; i < n; ++i) {
            decoded_deltas.push_back(GolombRiceDecode(bitreader, BASIC_FILTER_P));
        }
        Assert(stream.empty());
    }

    Assert(encoded_deltas == decoded_deltas);

    {
        const uint8_t p = fuzzed_data_provider.ConsumeIntegralInRange<uint8_t>(0, 64);
        const size_t n_values = fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 128);
        std::vector<uint64_t> values;
        values.reserve(n_values);
        for (size_t i = 0; i < n_values; ++i) {
            if (p == 64) {
                values.push_back(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            } else {
                const uint64_t max_quotient{std::min<uint64_t>(64, std::numeric_limits<uint64_t>::max() >> p)};
                const uint64_t quotient{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, max_quotient)};
                const uint64_t max_remainder{(uint64_t{1} << p) - 1};
                const uint64_t remainder{fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, max_remainder)};
                values.push_back((quotient << p) + remainder);
            }
        }
        AssertGolombRiceRoundTrip(values, p);
    }

    {
        const uint8_t p = fuzzed_data_provider.ConsumeIntegralInRange<uint8_t>(0, 64);
        const std::vector<uint8_t> random_bytes = ConsumeRandomLengthByteVector(fuzzed_data_provider, 1024);
        SpanReader stream{random_bytes};
        uint32_t n;
        try {
            n = static_cast<uint32_t>(ReadCompactSize(stream));
        } catch (const std::ios_base::failure&) {
            return;
        }
        BitStreamReader bitreader{stream};
        for (uint32_t i = 0; i < std::min<uint32_t>(n, 1024); ++i) {
            try {
                (void)GolombRiceDecode(bitreader, p);
            } catch (const std::ios_base::failure&) {
            }
        }
    }
}
