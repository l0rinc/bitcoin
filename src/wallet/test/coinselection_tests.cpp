// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <policy/policy.h>
#include <script/script.h>
#include <test/util/setup_common.h>
#include <util/check.h>
#include <util/vector.h>
#include <wallet/coinselection.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(coinselection_tests, BasicTestingSetup)

static int next_lock_time = 0;
static FastRandomContext default_rand;

static constexpr int P2WPKH_INPUT_VSIZE{68};
static constexpr int P2WPKH_OUTPUT_VSIZE{31};

/**
 * This set of feerates is used in the tests to test edge cases around the
 * default minimum feerate and other potential special cases:
 * - zero: 0 s/kvB
 * - minimum non-zero s/kvB: 1 s/kvB
 * - just below the new default minimum feerate: 99 s/kvB
 * - new default minimum feerate: 100 s/kvB
 * - old default minimum feerate: 1000 s/kvB
 * - a few non-round realistic feerates around default minimum feerate,
 * dust feerate, and default LTFRE: 315 s/kvB, 2345 s/kvB, and
 * 10'292 s/kvB
 * - a high feerate that has been exceeded occasionally: 59'764 s/kvB
 * - a huge feerate that is extremely uncommon: 1'500'000 s/kvB */
static const std::vector FEERATES = {0, 1, 99, 100, 315, 1'000, 2'345, 10'292, 59'764, 1'500'000};

/** Default coin selection parameters allow us to only explicitly set
 * parameters when a diverging value is relevant in the context of a test,
 * without reiterating the defaults in every test. We use P2WPKH input and
 * output weights for the change weights. */
static CoinSelectionParams init_cs_params(int eff_feerate = 5000)
{
    CoinSelectionParams csp{
        /*rng_fast=*/default_rand,
        /*change_output_size=*/P2WPKH_OUTPUT_VSIZE,
        /*change_spend_size=*/P2WPKH_INPUT_VSIZE,
        /*min_change_target=*/50'000,
        /*effective_feerate=*/CFeeRate(eff_feerate),
        /*long_term_feerate=*/CFeeRate(10'000),
        /*discard_feerate=*/CFeeRate(3000),
        /*tx_noinputs_size=*/11 + P2WPKH_OUTPUT_VSIZE, //static header size + output size
        /*avoid_partial=*/false,
    };
    csp.m_change_fee = csp.m_effective_feerate.GetFee(csp.change_output_size); // 155 sats for default feerate of 5000 s/kvB
    csp.min_viable_change = /*204 sats=*/csp.m_discard_feerate.GetFee(csp.change_spend_size);
    csp.m_cost_of_change = csp.min_viable_change + csp.m_change_fee; // 204 + 155 sats for default feerate of 5000 s/kvB
    csp.m_subtract_fee_outputs = false;
    return csp;
}

static const CoinSelectionParams default_cs_params = init_cs_params();

/** Make one OutputGroup with a single UTXO that has the given effective value */
static OutputGroup MakeCoin(const CAmount& amount, int input_size = P2WPKH_INPUT_VSIZE, const CoinSelectionParams& params = default_cs_params)
{
    CMutableTransaction tx;
    const CAmount fees{params.m_effective_feerate.GetFee(input_size)};
    tx.vout.emplace_back(amount + fees, CScript{});
    tx.nLockTime = next_lock_time++; // so all transactions get different hashes
    OutputGroup group(params);
    group.Insert(std::make_shared<COutput>(COutPoint(tx.GetHash(), 0), tx.vout.at(0), /*depth=*/1, /*input_bytes=*/input_size, /*solvable=*/true, /*safe=*/true, /*time=*/0, /*from_me=*/false, /*fees=*/fees), /*ancestors=*/0, /*cluster_count=*/0);
    return group;
}

/** Make multiple OutputGroups with the given values as their effective value */
static std::vector<OutputGroup> MakeCoins(const std::vector<CAmount>& coins, const CoinSelectionParams& params = default_cs_params, int input_size = P2WPKH_INPUT_VSIZE)
{
    std::vector<OutputGroup> utxo_pool;
    for (CAmount c : coins) {
        utxo_pool.push_back(MakeCoin(c, input_size, params));
    }
    return utxo_pool;
}

/** Make multiple coins that share the same effective value and input size */
static void AddDuplicateCoins(std::vector<OutputGroup>& utxo_pool, int count, CAmount amount, const CoinSelectionParams& params = default_cs_params, int input_size = P2WPKH_INPUT_VSIZE)
{
    for (int i{0}; i < count; ++i) {
        utxo_pool.push_back(MakeCoin(amount, input_size, params));
    }
}

static std::vector<std::pair<CAmount, int>> SortedInputs(const SelectionResult& selection)
{
    std::vector<std::pair<CAmount, int>> inputs;
    for (const auto& coin : selection.GetInputSet()) {
        inputs.emplace_back(coin->txout.nValue, coin->input_bytes);
    }
    std::ranges::sort(inputs);
    return inputs;
}

static SelectionResult MakeSelection(const std::vector<OutputGroup>& inputs)
{
    SelectionResult selection{0, SelectionAlgorithm::MANUAL};
    for (const auto& input : inputs) {
        selection.AddInput(input);
    }
    return selection;
}

// Expand assertions at the test call site so failures identify the scenario's source line
#define CHECK_SELECTION(result, expected_inputs, attempts, max_weight) \
    do { \
        BOOST_CHECK(SortedInputs(*Assert(result)) == SortedInputs(MakeSelection(expected_inputs))); \
        BOOST_CHECK_EQUAL((result)->GetSelectionsEvaluated(), (attempts)); \
        BOOST_CHECK_LE((result)->GetWeight(), (max_weight)); \
    } while (false)

// Exceeding the maximum weight is the only failure that carries an error message; insufficient funds fail silently
#define CHECK_SELECTION_FAILURE(result, overweight) \
    do { \
        BOOST_CHECK(!(result)); \
        BOOST_CHECK_EQUAL(!util::ErrorString(result).empty(), (overweight)); \
    } while (false)

// SRD must cover the target plus the minimum change and its fee
#define CHECK_SRD_SELECTION(result, target, params, max_weight) \
    do { \
        BOOST_CHECK_GE(Assert(result)->GetSelectedEffectiveValue(), (target) + (params).m_change_fee + CHANGE_LOWER); \
        BOOST_CHECK_LE((result)->GetWeight(), (max_weight)); \
    } while (false)

BOOST_AUTO_TEST_CASE(bnb_empty_pool)
{
    for (int feerate : FEERATES) {
        const auto cost_of_change{init_cs_params(feerate).m_cost_of_change};
        std::vector<OutputGroup> utxo_pool{};
        const auto result{SelectCoinsBnB(utxo_pool, /*selection_target=*/1 * CENT, cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION_FAILURE(result, /*overweight=*/false);
    }
}

BOOST_AUTO_TEST_CASE(bnb_basic_selection)
{
    const struct {
        CAmount target;
        std::vector<CAmount> expected;
        int attempts;
    } cases[]{
        {1 * CENT, {1 * CENT}, 3},
        {3 * CENT, {3 * CENT}, 3},
        {5 * CENT, {5 * CENT}, 2},
        {4 * CENT, {1 * CENT, 3 * CENT}, 4},
        {9 * CENT, {1 * CENT, 3 * CENT, 5 * CENT}, 5},
    };
    for (const auto& [target, expected, attempts] : cases) {
        for (int feerate : FEERATES) {
            const auto params{init_cs_params(feerate)};
            auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
            const auto result{SelectCoinsBnB(utxo_pool, target, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
            CHECK_SELECTION(result, MakeCoins(expected, params), attempts, MAX_STANDARD_TX_WEIGHT);
        }
    }
}

BOOST_AUTO_TEST_CASE(bnb_cost_of_change_boundary)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
        const CAmount target{4 * CENT - params.m_cost_of_change};
        const auto accepted{SelectCoinsBnB(utxo_pool, target, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION(accepted, MakeCoins({1 * CENT, 3 * CENT}, params), /*attempts=*/4, MAX_STANDARD_TX_WEIGHT);

        const auto rejected{SelectCoinsBnB(utxo_pool, target - 1, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION_FAILURE(rejected, /*overweight=*/false);
    }
}

BOOST_AUTO_TEST_CASE(bnb_weight_limit_boundary)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
        constexpr CAmount target{4 * CENT};
        constexpr int max_weight{4 * 2 * P2WPKH_INPUT_VSIZE};
        const auto accepted{SelectCoinsBnB(utxo_pool, target, params.m_cost_of_change, max_weight)};
        CHECK_SELECTION(accepted, MakeCoins({1 * CENT, 3 * CENT}, params), /*attempts=*/4, max_weight);

        const auto rejected{SelectCoinsBnB(utxo_pool, target, params.m_cost_of_change, max_weight - 1)};
        CHECK_SELECTION_FAILURE(rejected, /*overweight=*/true);
    }
}

BOOST_AUTO_TEST_CASE(bnb_no_solution)
{
    for (CAmount target : {CENT / 2, 7 * CENT, 10 * CENT}) {
        for (int feerate : FEERATES) {
            const auto params{init_cs_params(feerate)};
            auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
            const auto result{SelectCoinsBnB(utxo_pool, target, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
            CHECK_SELECTION_FAILURE(result, /*overweight=*/false);
        }
    }
}

BOOST_AUTO_TEST_CASE(bnb_skip_equivalent_input_sets)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeCoins({2 * CENT, 7 * CENT, 7 * CENT}, params)};
        AddDuplicateCoins(utxo_pool, /*count=*/50'000, /*amount=*/5 * CENT, params);
        const auto result{SelectCoinsBnB(utxo_pool, /*selection_target=*/16 * CENT, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION(result, MakeCoins({2 * CENT, 7 * CENT, 7 * CENT}, params), /*attempts=*/16, MAX_STANDARD_TX_WEIGHT);
    }
}

static constexpr int BNB_EXPECTED_INPUT_COUNT{8};

// Larger inputs exceed the target window, forcing BnB to explore combinations until it finds all eight small inputs
static auto MakeBnBAttemptPool(int input_count, const CoinSelectionParams& params)
{
    std::vector<OutputGroup> expected_inputs;
    for (int i{0}; i < BNB_EXPECTED_INPUT_COUNT; ++i) {
        expected_inputs.push_back(MakeCoin(CENT + i, P2WPKH_INPUT_VSIZE, params));
    }
    auto utxo_pool{expected_inputs};
    for (int i{BNB_EXPECTED_INPUT_COUNT}; i < input_count; ++i) {
        utxo_pool.push_back(MakeCoin(CENT + params.m_cost_of_change + i, P2WPKH_INPUT_VSIZE, params));
    }
    return std::pair{std::move(utxo_pool), std::move(expected_inputs)};
}

BOOST_AUTO_TEST_CASE(bnb_find_solution_before_attempt_limit)
{
    constexpr struct {
        int input_count;
        int attempts;
    } cases[]{
        {17, 51'765},
        {18, 87'957},
    };
    for (const auto& [input_count, attempts] : cases) {
        for (int feerate : FEERATES) {
            const auto params{init_cs_params(feerate)};
            auto [utxo_pool, expected_inputs]{MakeBnBAttemptPool(input_count, params)};
            const auto result{SelectCoinsBnB(utxo_pool, /*selection_target=*/BNB_EXPECTED_INPUT_COUNT * CENT, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
            CHECK_SELECTION(result, expected_inputs, attempts, MAX_STANDARD_TX_WEIGHT);
        }
    }
}

BOOST_AUTO_TEST_CASE(bnb_exhaust_with_19_inputs)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeBnBAttemptPool(19, params).first};
        const auto result{SelectCoinsBnB(utxo_pool, /*selection_target=*/BNB_EXPECTED_INPUT_COUNT * CENT, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION_FAILURE(result, /*overweight=*/false);
    }
}

BOOST_AUTO_TEST_CASE(bnb_exhaustion_with_solution_test)
{
    constexpr CAmount target{800'000};
    // A hard case with no exact-match solution: BnB must still report that the algorithm did not complete once the
    // search is pushed into the attempt limit, even though it finds a solution within cost_of_change of the target.
    std::vector<OutputGroup> utxo_pool(19);
    for (size_t i{0}; i < utxo_pool.size(); ++i) {
        utxo_pool[i] = MakeCoin(100'000 + i);
    }

    const auto result{Assert(SelectCoinsBnB(utxo_pool, target, default_cs_params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT))};
    BOOST_CHECK_GT(result->GetSelectedEffectiveValue(), target + 28);
    BOOST_CHECK_EQUAL(result->GetInputSet().size(), 8);
    BOOST_CHECK_EQUAL(result->GetSelectionsEvaluated(), 100'000);
    BOOST_CHECK(!result->GetAlgoCompleted());
}

BOOST_AUTO_TEST_CASE(bnb_feerate_sensitivity)
{
    const struct {
        CoinSelectionParams params;
        std::vector<CAmount> expected;
        int attempts;
    } cases[]{
        {default_cs_params, {2 * CENT, 3 * CENT, 5 * CENT}, 6},
        {init_cs_params(/*eff_feerate=*/25'000), {10 * CENT}, 5},
    };
    for (const auto& [params, expected, attempts] : cases) {
        auto utxo_pool{MakeCoins({2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT}, params)};
        const auto result{SelectCoinsBnB(utxo_pool, /*selection_target=*/10 * CENT, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION(result, MakeCoins(expected, params), attempts, MAX_STANDARD_TX_WEIGHT);
    }
}

BOOST_AUTO_TEST_CASE(bnb_mixed_weight_feerate_sensitivity)
{
    const struct {
        CoinSelectionParams params;
        std::vector<CAmount> expected;
        int input_size;
        int attempts;
    } cases[]{
        {default_cs_params, {6 * CENT, 7 * CENT}, 500, 18},
        {init_cs_params(/*eff_feerate=*/25'000), {3 * CENT, 10 * CENT}, P2WPKH_INPUT_VSIZE, 9},
    };
    for (const auto& [params, expected, input_size, attempts] : cases) {
        auto utxo_pool{Cat(MakeCoins({2 * CENT, 3 * CENT, 5 * CENT, 10 * CENT}, params),
                          MakeCoins({6 * CENT, 7 * CENT}, params, /*input_size=*/500))};
        const auto result{SelectCoinsBnB(utxo_pool, /*selection_target=*/13 * CENT, params.m_cost_of_change, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION(result, MakeCoins(expected, params, input_size), attempts, MAX_STANDARD_TX_WEIGHT);
    }
}

BOOST_AUTO_TEST_CASE(coin_grinder_empty_pool)
{
    std::vector<OutputGroup> utxo_pool;
    const auto result{CoinGrinder(utxo_pool, /*selection_target=*/1 * CENT, /*change_target=*/CENT, MAX_STANDARD_TX_WEIGHT)};
    CHECK_SELECTION_FAILURE(result, /*overweight=*/false);
}

BOOST_AUTO_TEST_CASE(coin_grinder_failures)
{
    constexpr struct {
        CAmount target;
        int max_weight;
        bool overweight;
    } cases[]{
        {4'950 * CENT, MAX_STANDARD_TX_WEIGHT, false},
        {2'950 * CENT, 1000, true},
    };
    for (const auto& [target, max_weight, overweight] : cases) {
        std::vector<OutputGroup> utxo_pool;
        AddDuplicateCoins(utxo_pool, /*count=*/10, /*amount=*/1 * COIN);
        AddDuplicateCoins(utxo_pool, /*count=*/10, /*amount=*/2 * COIN);
        const auto result{CoinGrinder(utxo_pool, target, /*change_target=*/CENT, max_weight)};
        CHECK_SELECTION_FAILURE(result, overweight);
    }
}

BOOST_AUTO_TEST_CASE(coin_grinder_lowest_weight_below_limit)
{
    std::vector<OutputGroup> utxo_pool;
    AddDuplicateCoins(utxo_pool, /*count=*/60, /*amount=*/33 * CENT);
    AddDuplicateCoins(utxo_pool, /*count=*/10, /*amount=*/2 * COIN);
    std::vector<OutputGroup> expected_inputs;
    AddDuplicateCoins(expected_inputs, /*count=*/10, /*amount=*/2 * COIN);
    AddDuplicateCoins(expected_inputs, /*count=*/17, /*amount=*/33 * CENT);
    constexpr int max_weight{10'000};
    const auto result{CoinGrinder(utxo_pool, /*selection_target=*/2'533 * CENT, /*change_target=*/CENT, max_weight)};
    CHECK_SELECTION(result, expected_inputs, /*attempts=*/37, max_weight);
}

BOOST_AUTO_TEST_CASE(coin_grinder_prefer_lighter_inputs)
{
    std::vector expected_inputs{MakeCoin(1 * COIN), MakeCoin(1 * COIN)};
    auto utxo_pool{expected_inputs};
    utxo_pool.push_back(MakeCoin(2 * COIN, /*input_size=*/148));
    const auto result{CoinGrinder(utxo_pool, /*selection_target=*/190 * CENT, /*change_target=*/CENT, MAX_STANDARD_TX_WEIGHT)};
    CHECK_SELECTION(result, expected_inputs, /*attempts=*/3, MAX_STANDARD_TX_WEIGHT);
}

BOOST_AUTO_TEST_CASE(coin_grinder_mixed_weights)
{
    auto utxo_pool{Cat(Cat(MakeCoins({3 * COIN, 6 * COIN, 9 * COIN, 12 * COIN, 15 * COIN}, default_cs_params, /*input_size=*/350),
                           MakeCoins({2 * COIN, 5 * COIN, 8 * COIN, 11 * COIN, 14 * COIN}, default_cs_params, /*input_size=*/250)),
                       MakeCoins({1 * COIN, 4 * COIN, 7 * COIN, 10 * COIN, 13 * COIN}, default_cs_params, /*input_size=*/150))};
    std::vector expected_inputs{MakeCoin(14 * COIN, /*input_size=*/250), MakeCoin(13 * COIN, /*input_size=*/150), MakeCoin(4 * COIN, /*input_size=*/150)};
    const auto result{CoinGrinder(utxo_pool, /*selection_target=*/30 * COIN, /*change_target=*/CENT, MAX_STANDARD_TX_WEIGHT)};
    CHECK_SELECTION(result, expected_inputs, /*attempts=*/92, MAX_STANDARD_TX_WEIGHT);
}

BOOST_AUTO_TEST_CASE(coin_grinder_lightest_among_clones)
{
    auto expected_inputs{MakeCoins({4 * COIN, 3 * COIN, 2 * COIN, 1 * COIN}, default_cs_params, /*input_size=*/100)};
    auto utxo_pool{expected_inputs};
    AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/8 * COIN, default_cs_params, /*input_size=*/1000);
    AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/7 * COIN, default_cs_params, /*input_size=*/800);
    AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/6 * COIN, default_cs_params, /*input_size=*/600);
    AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/5 * COIN, default_cs_params, /*input_size=*/400);
    const auto result{CoinGrinder(utxo_pool, /*selection_target=*/990 * CENT, /*change_target=*/CENT, MAX_STANDARD_TX_WEIGHT)};
    CHECK_SELECTION(result, expected_inputs, /*attempts=*/38, MAX_STANDARD_TX_WEIGHT);
}

BOOST_AUTO_TEST_CASE(coin_grinder_skip_tiny_inputs)
{
    std::vector expected_inputs{MakeCoin(1 * COIN, /*input_size=*/1000), MakeCoin(1 * COIN, /*input_size=*/1000)};
    auto utxo_pool{expected_inputs};
    utxo_pool.push_back(MakeCoin(180 * CENT, /*input_size=*/2500));
    for (int j = 0; j < 100; ++j) {
        utxo_pool.push_back(MakeCoin(CENT + j, /*input_size=*/110));
    }
    constexpr int max_weight{40'000};
    const auto result{CoinGrinder(utxo_pool, /*selection_target=*/190 * CENT, /*change_target=*/CENT, max_weight)};
    CHECK_SELECTION(result, expected_inputs, /*attempts=*/7, max_weight);
}

static constexpr int CG_EXPECTED_INPUT_COUNT{8};
static constexpr int CG_MAX_WEIGHT{CG_EXPECTED_INPUT_COUNT * P2WPKH_INPUT_VSIZE * WITNESS_SCALE_FACTOR};
static constexpr CAmount CG_SELECTION_TARGET{CG_EXPECTED_INPUT_COUNT * COIN - CENT};

// Distinct amounts prevent clone skipping. The larger, slightly heavier decoys are tried first, and any eight inputs that
// include a decoy exceed CG_MAX_WEIGHT, so the eight expected coins are the last combination CoinGrinder visits.
static auto MakeCoinGrinderAttemptPool(int input_count)
{
    std::vector<OutputGroup> expected_inputs;
    for (int i{0}; i < CG_EXPECTED_INPUT_COUNT; ++i) {
        expected_inputs.push_back(MakeCoin(COIN + i));
    }
    auto utxo_pool{expected_inputs};
    for (int i{CG_EXPECTED_INPUT_COUNT}; i < input_count; ++i) {
        utxo_pool.push_back(MakeCoin(COIN + i, /*input_size=*/P2WPKH_INPUT_VSIZE + 1));
    }
    return std::pair{std::move(utxo_pool), std::move(expected_inputs)};
}

BOOST_AUTO_TEST_CASE(coin_grinder_find_solution_before_attempt_limit)
{
    auto [utxo_pool, expected_inputs]{MakeCoinGrinderAttemptPool(18)};
    const auto result{CoinGrinder(utxo_pool, CG_SELECTION_TARGET, /*change_target=*/CENT, CG_MAX_WEIGHT)};
    CHECK_SELECTION(result, expected_inputs, /*attempts=*/63'692, CG_MAX_WEIGHT);
    BOOST_CHECK_EQUAL(result->GetWeight(), CG_MAX_WEIGHT);
    BOOST_CHECK(result->GetAlgoCompleted());
}

BOOST_AUTO_TEST_CASE(coin_grinder_exhaust_before_finding_solution)
{
    auto utxo_pool{MakeCoinGrinderAttemptPool(19).first};
    const auto result{CoinGrinder(utxo_pool, CG_SELECTION_TARGET, /*change_target=*/CENT, CG_MAX_WEIGHT)};
    // Every rejected input set included a heavier decoy, so exhaustion reports the weight limit
    CHECK_SELECTION_FAILURE(result, /*overweight=*/true);
}

BOOST_AUTO_TEST_CASE(srd_empty_pool)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        std::vector<OutputGroup> utxo_pool;
        const auto result{SelectCoinsSRD(utxo_pool, /*target_value=*/1 * CENT, params.m_change_fee, params.rng_fast, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION_FAILURE(result, /*overweight=*/false);
    }
}

BOOST_AUTO_TEST_CASE(srd_basic_selection)
{
    for (CAmount target : {CAmount{21'000}, 1 * CENT, CAmount{3'125'000}, 4 * CENT, 7 * CENT}) {
        for (int feerate : FEERATES) {
            const auto params{init_cs_params(feerate)};
            auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
            const auto result{SelectCoinsSRD(utxo_pool, target, params.m_change_fee, params.rng_fast, MAX_STANDARD_TX_WEIGHT)};
            CHECK_SRD_SELECTION(result, target, params, MAX_STANDARD_TX_WEIGHT);
        }
    }
}

BOOST_AUTO_TEST_CASE(srd_minimum_change_boundary)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
        const CAmount target{9 * CENT - params.m_change_fee - CHANGE_LOWER};
        const auto accepted{SelectCoinsSRD(utxo_pool, target, params.m_change_fee, params.rng_fast, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SRD_SELECTION(accepted, target, params, MAX_STANDARD_TX_WEIGHT);

        const auto rejected{SelectCoinsSRD(utxo_pool, target + 1, params.m_change_fee, params.rng_fast, MAX_STANDARD_TX_WEIGHT)};
        CHECK_SELECTION_FAILURE(rejected, /*overweight=*/false);
    }
}

BOOST_AUTO_TEST_CASE(srd_insufficient_funds)
{
    for (CAmount target : {9 * CENT + 1, 9 * CENT}) {
        for (int feerate : FEERATES) {
            const auto params{init_cs_params(feerate)};
            auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
            const auto result{SelectCoinsSRD(utxo_pool, target, params.m_change_fee, params.rng_fast, MAX_STANDARD_TX_WEIGHT)};
            CHECK_SELECTION_FAILURE(result, /*overweight=*/false);
        }
    }
}

BOOST_AUTO_TEST_CASE(srd_select_valuable_inputs_within_weight_limit)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
        AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/5 * CENT, params);
        AddDuplicateCoins(utxo_pool, /*count=*/3, /*amount=*/7 * CENT, params);
        constexpr int max_weight{4 * 4 * (P2WPKH_INPUT_VSIZE - 1)};
        constexpr CAmount target{20 * CENT};
        const auto result{SelectCoinsSRD(utxo_pool, target, params.m_change_fee, params.rng_fast, max_weight)};
        CHECK_SRD_SELECTION(result, target, params, max_weight);
    }
}

BOOST_AUTO_TEST_CASE(srd_no_selection_within_weight_limit)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        auto utxo_pool{MakeCoins({1 * CENT, 3 * CENT, 5 * CENT}, params)};
        AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/5 * CENT, params);
        AddDuplicateCoins(utxo_pool, /*count=*/3, /*amount=*/7 * CENT, params);
        const auto result{SelectCoinsSRD(utxo_pool, /*target_value=*/25 * CENT, params.m_change_fee, params.rng_fast, /*max_selection_weight=*/4 * 3 * P2WPKH_INPUT_VSIZE)};
        CHECK_SELECTION_FAILURE(result, /*overweight=*/true);
    }
}

BOOST_AUTO_TEST_CASE(srd_mixed_input_weights)
{
    for (int feerate : FEERATES) {
        const auto params{init_cs_params(feerate)};
        std::vector<OutputGroup> utxo_pool;
        AddDuplicateCoins(utxo_pool, /*count=*/100, /*amount=*/5 * CENT, params);
        utxo_pool.push_back(MakeCoin(5 * CENT, /*input_size=*/P2WPKH_INPUT_VSIZE - 1, params));
        constexpr int max_weight{4 * 3 * (P2WPKH_INPUT_VSIZE - 1)};
        constexpr CAmount target{9 * CENT};
        const auto result{SelectCoinsSRD(utxo_pool, target, params.m_change_fee, params.rng_fast, max_weight)};
        CHECK_SRD_SELECTION(result, target, params, max_weight);
    }
}

#undef CHECK_SELECTION
#undef CHECK_SELECTION_FAILURE
#undef CHECK_SRD_SELECTION

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
