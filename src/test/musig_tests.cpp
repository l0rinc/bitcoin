// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <musig.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <util/check.h>

#include <boost/test/unit_test.hpp>

#include <array>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(musig_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(nonce_generation_ignores_deterministic_rng)
{
    std::array<unsigned char, 32> key_bytes{};
    key_bytes.back() = 1;
    CKey key;
    key.Set(key_bytes.begin(), key_bytes.end(), /*fCompressedIn=*/true);
    const std::vector<CPubKey> pubkeys{key.GetPubKey()};
    const CPubKey aggregate_pubkey{*Assert(MuSig2AggregatePubkeys(pubkeys))};
    const uint256 sighash{uint256::ONE};

    const auto generate_nonce{[&] {
        MuSig2SecNonce secnonce;
        return CreateMuSig2Nonce(secnonce, sighash, key, aggregate_pubkey, pubkeys);
    }};

    SeedRandomForTest(SeedRand::ZEROS);
    const std::vector<uint8_t> first_nonce{generate_nonce()};
    SeedRandomForTest(SeedRand::ZEROS);
    const std::vector<uint8_t> second_nonce{generate_nonce()};
    BOOST_CHECK(first_nonce != second_nonce);
}

BOOST_AUTO_TEST_SUITE_END()
