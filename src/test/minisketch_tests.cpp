// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <minisketch.h>
#include <node/minisketchwrapper.h>
#include <random.h>
#include <test/util/common.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <utility>
#include <vector>

using node::MakeMinisketch32;
using node::MakeMinisketch32FP;

BOOST_FIXTURE_TEST_SUITE(minisketch_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(minisketch_test)
{
    for (int i = 0; i < 100; ++i) {
        uint32_t errors = 0 + m_rng.randrange(11);
        uint32_t start_a = 1 + m_rng.randrange(1000000000);
        uint32_t a_not_b = m_rng.randrange(errors + 1);
        uint32_t b_not_a = errors - a_not_b;
        uint32_t both = m_rng.randrange(10000);
        uint32_t end_a = start_a + a_not_b + both;
        uint32_t start_b = start_a + a_not_b;
        uint32_t end_b = start_b + both + b_not_a;

        Minisketch sketch_a = MakeMinisketch32(10);
        for (uint32_t a = start_a; a < end_a; ++a) sketch_a.Add(a);
        Minisketch sketch_b = MakeMinisketch32(10);
        for (uint32_t b = start_b; b < end_b; ++b) sketch_b.Add(b);

        Minisketch sketch_ar = MakeMinisketch32(10);
        Minisketch sketch_br = MakeMinisketch32(10);
        sketch_ar.Deserialize(sketch_a.Serialize());
        sketch_br.Deserialize(sketch_b.Serialize());

        Minisketch sketch_c = std::move(sketch_ar);
        sketch_c.Merge(sketch_br);
        auto dec = sketch_c.Decode(errors);
        BOOST_REQUIRE(dec.has_value());
        auto sols = std::move(*dec);
        std::sort(sols.begin(), sols.end());
        for (uint32_t i = 0; i < a_not_b; ++i) BOOST_CHECK_EQUAL(sols[i], start_a + i);
        for (uint32_t i = 0; i < b_not_a; ++i) BOOST_CHECK_EQUAL(sols[i + a_not_b], start_b + both + i);
    }
}

BOOST_AUTO_TEST_CASE(minisketch_wrapper_contracts)
{
    auto check_shape = [](const Minisketch& sketch, size_t capacity) {
        BOOST_REQUIRE(sketch);
        BOOST_CHECK_EQUAL(sketch.GetBits(), 32U);
        BOOST_CHECK_EQUAL(sketch.GetCapacity(), capacity);
        BOOST_CHECK_EQUAL(sketch.GetSerializedSize(), capacity * sizeof(uint32_t));
        BOOST_CHECK_EQUAL(sketch.Serialize().size(), sketch.GetSerializedSize());
    };

    Minisketch sketch_a{MakeMinisketch32(4)};
    check_shape(sketch_a, 4);

    Minisketch sketch_fp{MakeMinisketch32FP(/*max_elements=*/3, /*fpbits=*/8)};
    check_shape(sketch_fp, Minisketch::ComputeCapacity(32, /*max_elements=*/3, /*fpbits=*/8));
    sketch_fp.SetSeed(1).Add(42);
    check_shape(sketch_fp, Minisketch::ComputeCapacity(32, /*max_elements=*/3, /*fpbits=*/8));

    const auto empty_serialized{sketch_a.Serialize()};
    sketch_a.Add(0);
    BOOST_CHECK(sketch_a.Serialize() == empty_serialized);
    check_shape(sketch_a, 4);

    sketch_a.Add(1).Add(2).Add(3);
    check_shape(sketch_a, 4);
    Minisketch sketch_b{MakeMinisketch32(4)};
    sketch_b.Add(3).Add(4);
    check_shape(sketch_b, 4);

    Minisketch sketch_ar{MakeMinisketch32(4)};
    sketch_ar.Deserialize(sketch_a.Serialize());
    check_shape(sketch_ar, 4);
    BOOST_CHECK(sketch_ar.Serialize() == sketch_a.Serialize());

    Minisketch sketch_br{MakeMinisketch32(4)};
    sketch_br.Deserialize(sketch_b.Serialize());
    check_shape(sketch_br, 4);
    BOOST_CHECK(sketch_br.Serialize() == sketch_b.Serialize());

    Minisketch sketch_ab{sketch_ar};
    sketch_ab.Merge(sketch_br);
    check_shape(sketch_ab, 4);
    Minisketch sketch_ba{sketch_br};
    sketch_ba.Merge(sketch_ar);
    check_shape(sketch_ba, 4);
    BOOST_CHECK(sketch_ab.Serialize() == sketch_ba.Serialize());

    auto decoded{sketch_ab.Decode(4)};
    check_shape(sketch_ab, 4);
    BOOST_REQUIRE(decoded);
    std::sort(decoded->begin(), decoded->end());
    BOOST_CHECK(*decoded == std::vector<uint64_t>({1, 2, 4}));
}

BOOST_AUTO_TEST_SUITE_END()
