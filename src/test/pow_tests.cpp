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

#include <cassert>
#include <limits>
#include <optional>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

static void CheckPowTargetContracts(uint32_t nbits, const Consensus::Params& consensus)
{
    bool negative{false};
    bool overflow{false};
    arith_uint256 compact_target;
    compact_target.SetCompact(nbits, &negative, &overflow);

    CBlockHeader header;
    header.nBits = nbits;
    const arith_uint256 proof{GetBlockProof(header)};
    const std::optional<arith_uint256> derived_target{DeriveTarget(nbits, consensus.powLimit)};

    if (negative || overflow || compact_target == 0) {
        BOOST_CHECK(!derived_target);
        BOOST_CHECK(proof == 0);
        BOOST_CHECK(!CheckProofOfWorkImpl(uint256::ZERO, nbits, consensus));
        return;
    }

    const arith_uint256 expected_proof{(~compact_target / (compact_target + 1)) + 1};
    BOOST_CHECK(proof == expected_proof);

    const bool target_in_range{compact_target <= UintToArith256(consensus.powLimit)};
    BOOST_CHECK_EQUAL(derived_target.has_value(), target_in_range);
    if (!target_in_range) {
        BOOST_CHECK(!CheckProofOfWorkImpl(ArithToUint256(compact_target), nbits, consensus));
        return;
    }

    BOOST_CHECK(*derived_target == compact_target);
    BOOST_CHECK(CheckProofOfWorkImpl(uint256::ZERO, nbits, consensus));
    BOOST_CHECK(CheckProofOfWorkImpl(ArithToUint256(compact_target), nbits, consensus));
    BOOST_CHECK(!CheckProofOfWorkImpl(ArithToUint256(compact_target + 1), nbits, consensus));
}

static int64_t ExpectedEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& consensus)
{
    const arith_uint256 tip_proof{GetBlockProof(tip)};
    assert(tip_proof != 0);

    const bool to_has_more_work{to.nChainWork > from.nChainWork};
    const bool from_has_more_work{from.nChainWork > to.nChainWork};
    const arith_uint256 work_delta{
        to_has_more_work ? to.nChainWork - from.nChainWork : from.nChainWork - to.nChainWork};
    const arith_uint256 scaled_delta{
        work_delta * arith_uint256(consensus.nPowTargetSpacing) / tip_proof};
    const int64_t magnitude{
        scaled_delta.bits() > 63 ? std::numeric_limits<int64_t>::max() : int64_t(scaled_delta.GetLow64())};
    return from_has_more_work ? -magnitude : magnitude;
}

static void CheckEquivalentTimeContracts(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& consensus)
{
    BOOST_REQUIRE(GetBlockProof(tip) != 0);
    const int64_t expected{ExpectedEquivalentTime(to, from, tip, consensus)};
    const int64_t equivalent_time{GetBlockProofEquivalentTime(to, from, tip, consensus)};
    BOOST_CHECK_EQUAL(equivalent_time, expected);
    BOOST_CHECK_EQUAL(GetBlockProofEquivalentTime(from, to, tip, consensus), -expected);
    BOOST_CHECK_EQUAL(GetBlockProofEquivalentTime(to, to, tip, consensus), 0);
    if (to.nChainWork > from.nChainWork) BOOST_CHECK_GE(equivalent_time, 0);
    if (from.nChainWork > to.nChainWork) BOOST_CHECK_LE(equivalent_time, 0);
}

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

BOOST_AUTO_TEST_CASE(pow_target_contracts)
{
    const auto consensus = CreateChainParams(*m_node.args, ChainType::MAIN)->GetConsensus();

    CheckPowTargetContracts(UintToArith256(consensus.powLimit).GetCompact(), consensus);
    CheckPowTargetContracts(0x1c05a3f4, consensus);
    CheckPowTargetContracts(UintToArith256(consensus.powLimit).GetCompact(true), consensus);
    CheckPowTargetContracts(~0x00800000U, consensus);
    CheckPowTargetContracts(0, consensus);

    arith_uint256 too_easy{UintToArith256(consensus.powLimit)};
    too_easy *= 2;
    CheckPowTargetContracts(too_easy.GetCompact(), consensus);
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

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_contracts)
{
    const auto chainParams = CreateChainParams(*m_node.args, ChainType::MAIN);
    const auto& consensus{chainParams->GetConsensus()};

    CBlockIndex tip;
    tip.nBits = 0x1d00ffff;
    const arith_uint256 tip_proof{GetBlockProof(tip)};
    BOOST_REQUIRE(tip_proof != 0);

    CBlockIndex lower_work;
    lower_work.nChainWork = arith_uint256{7};
    CBlockIndex higher_work;
    higher_work.nChainWork = lower_work.nChainWork + tip_proof * arith_uint256{3};
    CheckEquivalentTimeContracts(higher_work, lower_work, tip, consensus);

    CBlockIndex same_work;
    same_work.nChainWork = lower_work.nChainWork;
    CheckEquivalentTimeContracts(same_work, lower_work, tip, consensus);

    CBlockIndex maximum_work;
    maximum_work.nChainWork = ~arith_uint256{0};
    CBlockIndex no_work;
    no_work.nChainWork = arith_uint256{0};
    CheckEquivalentTimeContracts(maximum_work, no_work, tip, consensus);
    BOOST_CHECK_EQUAL(GetBlockProofEquivalentTime(maximum_work, no_work, tip, consensus), std::numeric_limits<int64_t>::max());

    CBlockIndex invalid_tip;
    invalid_tip.nBits = 0;
    BOOST_CHECK_THROW(GetBlockProofEquivalentTime(higher_work, lower_work, invalid_tip, consensus), uint_error);
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
