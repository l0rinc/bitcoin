// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <minisketch.h>
#include <node/minisketchwrapper.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/check.h>

#include <algorithm>
#include <map>
#include <numeric>
#include <vector>

namespace {

Minisketch MakeFuzzMinisketch32(size_t capacity, uint32_t impl)
{
    return Assert(Minisketch(32, impl, capacity));
}

void AssertSketchShape(const Minisketch& sketch, size_t capacity, uint32_t impl)
{
    Assert(sketch.GetBits() == 32);
    Assert(sketch.GetCapacity() == capacity);
    Assert(sketch.GetImplementation() == impl);
    Assert(sketch.GetSerializedSize() == capacity * sizeof(uint32_t));
    Assert(sketch.Serialize().size() == sketch.GetSerializedSize());
}

} // namespace

FUZZ_TARGET(minisketch)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};

    const auto capacity{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 200)};
    const uint32_t impl{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(0, Minisketch::MaxImplementation())};
    if (!Minisketch::ImplementationSupported(32, impl)) return;

    Minisketch sketch_a{MakeFuzzMinisketch32(capacity, impl)};
    Minisketch sketch_b{MakeFuzzMinisketch32(capacity, impl)};
    AssertSketchShape(sketch_a, capacity, impl);
    AssertSketchShape(sketch_b, capacity, impl);

    const auto empty_serialized{sketch_a.Serialize()};
    sketch_a.Add(0);
    Assert(sketch_a.Serialize() == empty_serialized);
    AssertSketchShape(sketch_a, capacity, impl);

    sketch_a.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    sketch_b.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    AssertSketchShape(sketch_a, capacity, impl);
    AssertSketchShape(sketch_b, capacity, impl);

    // Fill two sets and keep the difference in a map
    std::map<uint32_t, bool> diff;
    LIMITED_WHILE (fuzzed_data_provider.ConsumeBool(), 10000) {
        const auto entry{fuzzed_data_provider.ConsumeIntegralInRange<uint32_t>(1, std::numeric_limits<uint32_t>::max() - 1)};
        const auto KeepDiff{[&] {
            bool& mut{diff[entry]};
            mut = !mut;
        }};
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                sketch_a.Add(entry);
                AssertSketchShape(sketch_a, capacity, impl);
                KeepDiff();
            },
            [&] {
                sketch_b.Add(entry);
                AssertSketchShape(sketch_b, capacity, impl);
                KeepDiff();
            },
            [&] {
                sketch_a.Add(entry);
                sketch_b.Add(entry);
                AssertSketchShape(sketch_a, capacity, impl);
                AssertSketchShape(sketch_b, capacity, impl);
            });
    }
    const auto num_diff{std::accumulate(diff.begin(), diff.end(), size_t{0}, [](auto n, const auto& e) { return n + e.second; })};
    std::vector<uint64_t> expected_diff;
    expected_diff.reserve(num_diff);
    for (const auto& [entry, in_diff] : diff) {
        if (in_diff) expected_diff.push_back(entry);
    }
    Assert(expected_diff.size() == num_diff);

    Minisketch sketch_ar{MakeFuzzMinisketch32(capacity, impl)};
    Minisketch sketch_br{MakeFuzzMinisketch32(capacity, impl)};
    sketch_ar.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    sketch_br.SetSeed(fuzzed_data_provider.ConsumeIntegral<uint64_t>());
    AssertSketchShape(sketch_ar, capacity, impl);
    AssertSketchShape(sketch_br, capacity, impl);

    sketch_ar.Deserialize(sketch_a.Serialize());
    sketch_br.Deserialize(sketch_b.Serialize());
    AssertSketchShape(sketch_ar, capacity, impl);
    AssertSketchShape(sketch_br, capacity, impl);
    Assert(sketch_ar.Serialize() == sketch_a.Serialize());
    Assert(sketch_br.Serialize() == sketch_b.Serialize());

    Minisketch sketch_ab{sketch_ar};
    sketch_ab.Merge(sketch_br);
    AssertSketchShape(sketch_ab, capacity, impl);
    Minisketch sketch_ba{sketch_br};
    sketch_ba.Merge(sketch_ar);
    AssertSketchShape(sketch_ba, capacity, impl);
    Assert(sketch_ab.Serialize() == sketch_ba.Serialize());

    Minisketch sketch_diff{fuzzed_data_provider.ConsumeBool() ? sketch_a : sketch_ar};
    sketch_diff.Merge(fuzzed_data_provider.ConsumeBool() ? sketch_b : sketch_br);
    AssertSketchShape(sketch_diff, capacity, impl);
    Assert(sketch_diff.Serialize() == sketch_ab.Serialize());

    if (capacity >= num_diff) {
        const auto max_elements{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(num_diff, capacity)};
        const auto dec{*Assert(sketch_diff.Decode(max_elements))};
        AssertSketchShape(sketch_diff, capacity, impl);
        auto sorted_dec{dec};
        std::sort(sorted_dec.begin(), sorted_dec.end());
        Assert(sorted_dec == expected_diff);
    }
}
