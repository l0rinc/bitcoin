// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/check.h>
#include <util/overflow.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

FUZZ_TARGET(fee_rate)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const CAmount satoshis_per_k = ConsumeMoney(fuzzed_data_provider);
    const CFeeRate fee_rate{satoshis_per_k};

    (void)fee_rate.GetFeePerK();
    const auto bytes = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, std::numeric_limits<int32_t>::max());
    if (!MultiplicationOverflow(int64_t{bytes}, satoshis_per_k)) {
        (void)fee_rate.GetFee(bytes);
    }
    (void)fee_rate.ToString();

    {
        const CAmount precise_fee{fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-MAX_MONEY, MAX_MONEY)};
        const int32_t precise_vsize{fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(1, std::numeric_limits<int32_t>::max())};
        const CFeeRate precise_rate{precise_fee, precise_vsize};
        Assert(precise_rate.GetFee(precise_vsize) == precise_fee);

        CFeeRate add_zero{precise_rate};
        add_zero += CFeeRate{0};
        Assert(add_zero == precise_rate);

        CFeeRate default_zero;
        default_zero += precise_rate;
        Assert(default_zero == precise_rate);

        CFeeRate integer_zero{0};
        integer_zero += precise_rate;
        Assert(integer_zero == precise_rate);
    }

    {
        const CAmount multiplier_fee{fuzzed_data_provider.ConsumeIntegral<CAmount>()};
        const int multiplier{fuzzed_data_provider.ConsumeIntegral<int>()};
        const CFeeRate multiplier_rate{multiplier_fee};
        const CFeeRate expected_rate{SaturatingMul(multiplier_fee, int64_t{multiplier})};
        Assert(multiplier_rate * multiplier == expected_rate);
        Assert(multiplier * multiplier_rate == expected_rate);
        Assert(multiplier_rate * multiplier == multiplier * multiplier_rate);
        Assert(multiplier_rate * 0 == CFeeRate{0});
        Assert(0 * multiplier_rate == CFeeRate{0});
    }

    const CAmount another_satoshis_per_k = ConsumeMoney(fuzzed_data_provider);
    CFeeRate larger_fee_rate{another_satoshis_per_k};
    larger_fee_rate += fee_rate;
    if (satoshis_per_k != 0 && another_satoshis_per_k != 0) {
        assert(fee_rate < larger_fee_rate);
        assert(!(fee_rate > larger_fee_rate));
        assert(!(fee_rate == larger_fee_rate));
        assert(fee_rate <= larger_fee_rate);
        assert(!(fee_rate >= larger_fee_rate));
        assert(fee_rate != larger_fee_rate);
    }
}
