// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/random.h>
#include <test/util/common.h>
#include <test/util/setup_common.h>
#include <util/chaintype.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1261130161; // Block #30240
    CBlockIndex pindexLast;
    pindexLast.nHeight = 32255;
    pindexLast.nTime = 1262152739;  // Block #32255
    pindexLast.nBits = 0x1d00ffff;

    // Here (and below): expected_nbits is calculated in
    // CalculateNextWorkRequired(); redoing the calculation here would be just
    // reimplementing the same code that is written in pow.cpp. Rather than
    // copy that code, we just hardcode the expected result.
    unsigned int expected_nbits = 0x1d00d86aU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1231006505; // Block #0
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2015;
    pindexLast.nTime = 1233061996;  // Block #2015
    pindexLast.nBits = 0x1d00ffff;
    unsigned int expected_nbits = 0x1d00ffffU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
}

BOOST_AUTO_TEST_CASE(get_next_work_uses_current_pow_limit)
{
    auto params{CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus()};
    params.enforce_BIP94 = false;
    params.fPowAllowMinDifficultyBlocks = false;
    params.fPowNoRetargeting = false;
    params.nPowTargetSpacing = 10 * 60;
    params.nPowTargetTimespan = 14 * 24 * 60 * 60;

    constexpr uint32_t low_pow_limit_bits{0x1d00ffffU};
    constexpr uint32_t high_pow_limit_bits{0x207fffffU};
    const auto pow_limit_from_bits = [](uint32_t bits) {
        return ArithToUint256(arith_uint256{}.SetCompact(bits));
    };

    CBlockIndex pindexLast;
    pindexLast.nHeight = params.DifficultyAdjustmentInterval() - 1;
    pindexLast.nTime = params.nPowTargetTimespan * 4;
    pindexLast.nBits = low_pow_limit_bits;

    auto high_limit_params{params};
    high_limit_params.powLimit = pow_limit_from_bits(high_pow_limit_bits);
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, 0, high_limit_params), 0x1d03fffcU);

    auto low_limit_params{params};
    low_limit_params.powLimit = pow_limit_from_bits(low_pow_limit_bits);
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, 0, low_limit_params), low_pow_limit_bits);
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1279008237; // Block #66528
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68543;
    pindexLast.nTime = 1279297671;  // Block #68543
    pindexLast.nBits = 0x1c05a3f4;
    unsigned int expected_nbits = 0x1c0168fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that reducing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits-1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    int64_t nLastRetargetTime = 1263163443; // NOTE: Not an actual block time
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46367;
    pindexLast.nTime = 1269211443;  // Block #46367
    pindexLast.nBits = 0x1c387f6f;
    unsigned int expected_nbits = 0x1d00e1fdU;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), expected_nbits);
    BOOST_CHECK(PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, expected_nbits));
    // Test that increasing nbits further would not be a PermittedDifficultyTransition.
    unsigned int invalid_nbits = expected_nbits+1;
    BOOST_CHECK(!PermittedDifficultyTransition(chainParams->GetConsensus(), pindexLast.nHeight+1, pindexLast.nBits, invalid_nbits));
}

BOOST_AUTO_TEST_CASE(get_next_work_bip94_uses_first_period_bits)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET4);
    const auto consensus = chainParams->GetConsensus();
    BOOST_REQUIRE(consensus.enforce_BIP94);
    BOOST_REQUIRE(consensus.fPowAllowMinDifficultyBlocks);

    const int interval{static_cast<int>(consensus.DifficultyAdjustmentInterval())};
    const unsigned int first_period_bits{0x1c0ffff0U};
    const unsigned int pow_limit_bits{UintToArith256(consensus.powLimit).GetCompact()};
    const int64_t first_time{1700000000};

    std::vector<CBlockIndex> blocks(interval);
    for (int i = 0; i < interval; ++i) {
        blocks[i].nHeight = interval + i;
        blocks[i].pprev = i > 0 ? &blocks[i - 1] : nullptr;
        blocks[i].nTime = first_time + i * consensus.nPowTargetSpacing;
        blocks[i].nBits = pow_limit_bits;
    }
    blocks.front().nBits = first_period_bits;
    blocks.back().nTime = first_time + consensus.nPowTargetTimespan;

    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&blocks.back(), blocks.front().GetBlockTime(), consensus), first_period_bits);

    auto legacy_consensus{consensus};
    legacy_consensus.enforce_BIP94 = false;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&blocks.back(), blocks.front().GetBlockTime(), legacy_consensus), pow_limit_bits);
}

BOOST_AUTO_TEST_CASE(get_next_work_testnet_easy_boundary_uses_retarget_bits)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::TESTNET);
    const auto consensus = chainParams->GetConsensus();
    BOOST_REQUIRE(consensus.fPowAllowMinDifficultyBlocks);
    BOOST_REQUIRE(!consensus.enforce_BIP94);

    const int interval{static_cast<int>(consensus.DifficultyAdjustmentInterval())};
    const unsigned int boundary_bits{0x1c0ffff0U};
    const unsigned int pow_limit_bits{UintToArith256(consensus.powLimit).GetCompact()};
    const int64_t boundary_time{1700000000};

    CBlockIndex boundary;
    boundary.nHeight = interval;
    boundary.nTime = boundary_time;
    boundary.nBits = boundary_bits;

    CBlockIndex parent;
    parent.nHeight = interval + 1;
    parent.pprev = &boundary;
    parent.nTime = boundary_time + consensus.nPowTargetSpacing * 3;
    parent.nBits = pow_limit_bits;

    CBlockHeader child;
    child.nTime = parent.GetBlockTime() + consensus.nPowTargetSpacing;

    BOOST_CHECK_EQUAL(GetNextWorkRequired(&parent, &child, consensus), boundary_bits);
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits{~0x00800000U};
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash = uint256{1};
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_accepts_compact_exponent_34_target)
{
    auto consensus{CreateChainParams(*m_node.args, ChainType::REGTEST)->GetConsensus()};
    constexpr uint32_t exponent_34_bits{0x22000001U};
    bool is_negative{true};
    bool is_overflow{true};
    arith_uint256 target;
    target.SetCompact(exponent_34_bits, &is_negative, &is_overflow);

    BOOST_CHECK(!is_negative);
    BOOST_CHECK(!is_overflow);
    BOOST_CHECK(target != 0);
    BOOST_CHECK(target <= UintToArith256(consensus.powLimit));
    BOOST_CHECK(CheckProofOfWork(uint256::ZERO, exponent_34_bits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_uses_current_pow_limit)
{
    const auto consensus{CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus()};
    constexpr uint32_t low_pow_limit_bits{0x1d00ffffU};
    constexpr uint32_t high_pow_limit_bits{0x207fffffU};
    constexpr uint32_t between_limits_bits{0x1d03fffcU};
    const auto pow_limit_from_bits = [](uint32_t bits) {
        return ArithToUint256(arith_uint256{}.SetCompact(bits));
    };

    auto high_limit_consensus{consensus};
    high_limit_consensus.powLimit = pow_limit_from_bits(high_pow_limit_bits);
    BOOST_CHECK(CheckProofOfWork(uint256::ZERO, between_limits_bits, high_limit_consensus));

    auto low_limit_consensus{consensus};
    low_limit_consensus.powLimit = pow_limit_from_bits(low_pow_limit_bits);
    BOOST_CHECK(!CheckProofOfWork(uint256::ZERO, between_limits_bits, low_limit_consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[m_rng.randrange(10000)];
        CBlockIndex *p2 = &blocks[m_rng.randrange(10000)];
        CBlockIndex *p3 = &blocks[m_rng.randrange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

void sanity_check_chainparams(const ArgsManager& args, ChainType chain_type)
{
    const auto chainParams = CreateChainParams(args, chain_type);
    const auto consensus = chainParams->GetConsensus();

    // hash genesis is correct
    BOOST_CHECK_EQUAL(consensus.hashGenesisBlock, chainParams->GenesisBlock().GetHash());

    // target timespan is an even multiple of spacing
    BOOST_CHECK_EQUAL(consensus.nPowTargetTimespan % consensus.nPowTargetSpacing, 0);

    // genesis nBits is positive, doesn't overflow and is lower than powLimit
    arith_uint256 pow_compact;
    bool neg, over;
    pow_compact.SetCompact(chainParams->GenesisBlock().nBits, &neg, &over);
    BOOST_CHECK(!neg && pow_compact != 0);
    BOOST_CHECK(!over);
    BOOST_CHECK(UintToArith256(consensus.powLimit) >= pow_compact);

    // check max target * 4*nPowTargetTimespan doesn't overflow -- see pow.cpp:CalculateNextWorkRequired()
    if (!consensus.fPowNoRetargeting) {
        arith_uint256 targ_max{UintToArith256(uint256{"ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"})};
        targ_max /= consensus.nPowTargetTimespan*4;
        BOOST_CHECK(UintToArith256(consensus.powLimit) < targ_max);
    }
}

BOOST_AUTO_TEST_CASE(ChainParams_MAIN_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::MAIN);
}

BOOST_AUTO_TEST_CASE(ChainParams_REGTEST_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::REGTEST);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET);
}

BOOST_AUTO_TEST_CASE(ChainParams_TESTNET4_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::TESTNET4);
}

BOOST_AUTO_TEST_CASE(ChainParams_SIGNET_sanity)
{
    sanity_check_chainparams(*m_node.args, ChainType::SIGNET);
}

BOOST_AUTO_TEST_SUITE_END()
