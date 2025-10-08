// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <coins.h>
#include <consensus/amount.h>
#include <consensus/tx_check.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <util/hasher.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void initialize_coins_view()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

void TestCoinsView(FuzzedDataProvider& fuzzed_data_provider, CCoinsView& backend_coins_view, bool is_db)
{
    bool good_data{true};

    CCoinsViewCache coins_view_cache{&backend_coins_view, /*deterministic=*/true};
    if (is_db) coins_view_cache.SetBestBlock(uint256::ONE);
    COutPoint random_out_point;
    Coin random_coin;
    CMutableTransaction random_mutable_transaction;
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 10'000)
    {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                if (random_coin.IsSpent()) {
                    return;
                }
                Coin coin = random_coin;
                bool expected_code_path = false;
                const bool possible_overwrite = fuzzed_data_provider.ConsumeBool();
                try {
                    coins_view_cache.AddCoin(random_out_point, std::move(coin), possible_overwrite);
                    expected_code_path = true;
                } catch (const std::logic_error& e) {
                    if (e.what() == std::string{"Attempted to overwrite an unspent coin (when possible_overwrite is false)"}) {
                        assert(!possible_overwrite);
                        expected_code_path = true;
                        // AddCoin() decreases cachedCoinsUsage by the memory usage of the old coin at the beginning and
                        // increases it by the value of the new coin at the end. If it throws in the process, the value
                        // of cachedCoinsUsage would have been incorrectly decreased, leading to an underflow later on.
                        // To avoid this, use Flush() to reset the value of cachedCoinsUsage in sync with the cacheCoins
                        // mapping.
                        (void)coins_view_cache.Flush();
                    }
                }
                assert(expected_code_path);
            },
            [&] {
            },
            [&] {
            },
            [&] {
            },
            [&] {
            },
            [&] {
            },
            [&] {
            },
            [&] {
            },
            [&] {
                const std::optional<Coin> opt_coin = ConsumeDeserializable<Coin>(fuzzed_data_provider);
                if (!opt_coin) {
                    good_data = false;
                    return;
                }
                const Coin& c = *opt_coin;
                std::cout << "opt_coin = " << strprintf("Coin{height=%u, fCoinBase=%u, %s}", c.nHeight, c.fCoinBase, c.out.ToString()) << std::endl;
                random_coin = c;
            },
            [&] {
            },
            [&] {
            });
    }
}

FUZZ_TARGET(coins_view, .init = initialize_coins_view)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CCoinsView backend_coins_view;
    TestCoinsView(fuzzed_data_provider, backend_coins_view, /*is_db=*/false);
}

FUZZ_TARGET(coins_view_db, .init = initialize_coins_view)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    auto db_params = DBParams{
        .path = "",
        .cache_bytes = 1_MiB,
        .memory_only = true,
    };
    CCoinsViewDB coins_db{std::move(db_params), CoinsViewOptions{}};
    TestCoinsView(fuzzed_data_provider, coins_db, /*is_db=*/true);
}
