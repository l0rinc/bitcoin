// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <array>
#include <cassert>
#include <compare>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace {

using boost::multiprecision::cpp_int;

/** Independent signed floor/ceiling arithmetic for fee/size ratios. */
cpp_int DivFloor(const cpp_int& numerator, const cpp_int& denominator)
{
    assert(denominator > 0);
    if (numerator >= 0) return numerator / denominator;
    return -((-numerator + denominator - 1) / denominator);
}

cpp_int DivCeil(const cpp_int& numerator, const cpp_int& denominator)
{
    assert(denominator > 0);
    if (numerator >= 0) return (numerator + denominator - 1) / denominator;
    return numerator / denominator;
}

CAmount ToAmount(const cpp_int& value)
{
    assert(value >= std::numeric_limits<CAmount>::min());
    assert(value <= std::numeric_limits<CAmount>::max());
    return value.convert_to<CAmount>();
}

CAmount ExpectedFee(const CAmount fee, const int32_t virtual_bytes, const int32_t size)
{
    assert(virtual_bytes >= 0);
    assert(size > 0);
    CAmount result = ToAmount(DivCeil(cpp_int{fee} * virtual_bytes, size));
    // CFeeRate deliberately reports a one-satoshi fee for a negative, non-zero
    // rate whose mathematical ceiling is zero.
    if (result == 0 && virtual_bytes != 0 && fee < 0) result = -1;
    return result;
}

CAmount ExpectedFeePerK(const CAmount fee, const int32_t size)
{
    assert(size > 0);
    return ToAmount(DivFloor(cpp_int{fee} * 1000, size));
}

std::strong_ordering ExpectedRateOrder(CAmount fee_a, int32_t size_a, CAmount fee_b, int32_t size_b)
{
    // ByRatio intentionally leaves the empty rate undefined: it is neither
    // lower nor higher than a non-empty rate, so the rate-only comparison is
    // equivalent. ByRatioNegSize is the separate wrapper that orders empty
    // values last.
    if (size_a <= 0 || size_b <= 0) return std::strong_ordering::equal;
    const cpp_int left{cpp_int{fee_a} * size_b};
    const cpp_int right{cpp_int{fee_b} * size_a};
    if (left < right) return std::strong_ordering::less;
    if (left > right) return std::strong_ordering::greater;
    return std::strong_ordering::equal;
}

std::string ExpectedToString(const CAmount fee_per_k, const FeeRateFormat format)
{
    assert(fee_per_k >= 0);
    std::ostringstream result;
    switch (format) {
    case FeeRateFormat::BTC_KVB:
        result << fee_per_k / COIN << '.' << std::setw(8) << std::setfill('0') << fee_per_k % COIN << " BTC/kvB";
        return result.str();
    case FeeRateFormat::SAT_VB:
        result << fee_per_k / 1000 << '.' << std::setw(3) << std::setfill('0') << fee_per_k % 1000 << " sat/vB";
        return result.str();
    }
    assert(false);
}

void AssertIntegralRate(const CAmount fee_per_k, const int32_t virtual_bytes)
{
    const CFeeRate rate{fee_per_k};
    assert((rate.GetFeePerVSize() == FeePerVSize{fee_per_k, 1000}));
    assert(rate.GetFeePerK() == fee_per_k);
    assert(rate.GetFee(0) == 0);
    if (!MultiplicationOverflow(int64_t{virtual_bytes}, fee_per_k)) {
        assert(rate.GetFee(virtual_bytes) == ExpectedFee(fee_per_k, virtual_bytes, 1000));
    }
}

void AssertRationalRateAtSize(const CAmount fee, const int32_t size, const int32_t at_size)
{
    const CFeeRate rate{fee, size};
    if (size <= 0) {
        assert(rate.GetFeePerVSize().IsEmpty());
        assert(rate == CFeeRate{0});
        assert(rate.GetFee(0) == 0);
        return;
    }

    assert((rate.GetFeePerVSize() == FeePerVSize{fee, size}));
    assert(rate.GetFeePerK() == ExpectedFeePerK(fee, size));
    assert(rate.GetFee(at_size) == ExpectedFee(fee, at_size, size));
    assert(rate.GetFee(0) == 0);
}

void AssertRationalRate(const CAmount fee, const int32_t size, FuzzedDataProvider& provider)
{
    const int32_t at_size = size > 0 ? provider.ConsumeIntegralInRange<int32_t>(0, size) : 0;
    AssertRationalRateAtSize(fee, size, at_size);
}

void AssertOrdering(const CAmount fee_a, const int32_t size_a, const CAmount fee_b, const int32_t size_b)
{
    const CFeeRate a{fee_a, size_a};
    const CFeeRate b{fee_b, size_b};
    const auto expected = ExpectedRateOrder(fee_a, size_a, fee_b, size_b);
    assert((a <=> b) == expected);
    assert((a == b) == std::is_eq(expected));
    assert((a != b) == std::is_neq(expected));
    assert((a < b) == std::is_lt(expected));
    assert((a <= b) == std::is_lteq(expected));
    assert((a > b) == std::is_gt(expected));
    assert((a >= b) == std::is_gteq(expected));
}

void AssertBoundaryContracts()
{
    constexpr std::array<CAmount, 8> fees{0, 1, 123, 999, 1000, MAX_MONEY, -1, -123};
    constexpr std::array<int32_t, 8> sizes{0, 1, 7, 8, 9, 999, 1000, std::numeric_limits<int32_t>::max()};

    for (const CAmount fee : fees) {
        const CFeeRate rate{fee};
        assert(rate.GetFeePerK() == fee);
        if (fee >= 0) {
            assert(rate.ToString() == ExpectedToString(fee, FeeRateFormat::BTC_KVB));
            assert(rate.ToString(FeeRateFormat::SAT_VB) == ExpectedToString(fee, FeeRateFormat::SAT_VB));
        }
        for (const int32_t size : sizes) {
            if (!MultiplicationOverflow(int64_t{size}, fee)) {
                AssertIntegralRate(fee, size);
            }
        }
    }

    // This exercises the high-magnitude multiplication path while keeping the
    // final fee representable in CAmount.
    const CFeeRate maximum{MAX_MONEY, std::numeric_limits<int32_t>::max()};
    assert(maximum.GetFeePerK() == ExpectedFeePerK(MAX_MONEY, std::numeric_limits<int32_t>::max()));
    assert(maximum.GetFee(std::numeric_limits<int32_t>::max()) == MAX_MONEY);
    assert(maximum.GetFee(std::numeric_limits<int32_t>::max() - 1) ==
           ExpectedFee(MAX_MONEY, std::numeric_limits<int32_t>::max() - 1, std::numeric_limits<int32_t>::max()));
    AssertRationalRateAtSize(1, 1001, 1);
    AssertRationalRateAtSize(2, 1001, 1);
    AssertRationalRateAtSize(-1, 1001, 1);
    AssertRationalRateAtSize(1, 0, 0);

    AssertOrdering(1, 3, 2, 6);
    AssertOrdering(1, 3, 2, 5);
    AssertOrdering(0, 0, 0, 1000);
    AssertOrdering(0, 0, 1, 1000);
    AssertOrdering(-1, 3, 0, 1000);
    AssertOrdering(MAX_MONEY, std::numeric_limits<int32_t>::max(), MAX_MONEY - 1, std::numeric_limits<int32_t>::max() - 1);
}

} // namespace

FUZZ_TARGET(fee_rate)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    AssertBoundaryContracts();

    const CAmount satoshis_per_k = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-MAX_MONEY, MAX_MONEY);
    const CFeeRate fee_rate{satoshis_per_k};

    const auto bytes = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(0, std::numeric_limits<int32_t>::max());
    if (!MultiplicationOverflow(int64_t{bytes}, satoshis_per_k)) {
        assert(fee_rate.GetFee(bytes) == ExpectedFee(satoshis_per_k, bytes, 1000));
    }
    assert(fee_rate.GetFeePerK() == satoshis_per_k);

    const CAmount another_satoshis_per_k = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-MAX_MONEY, MAX_MONEY);
    CFeeRate larger_fee_rate{another_satoshis_per_k};
    larger_fee_rate += fee_rate;
    assert(larger_fee_rate.GetFeePerK() == another_satoshis_per_k + satoshis_per_k);
    assert(larger_fee_rate == CFeeRate{another_satoshis_per_k + satoshis_per_k});

    const CAmount paid = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-MAX_MONEY, MAX_MONEY);
    const int32_t virtual_bytes = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(-1, std::numeric_limits<int32_t>::max());
    AssertRationalRate(paid, virtual_bytes, fuzzed_data_provider);

    const CAmount other_paid = fuzzed_data_provider.ConsumeIntegralInRange<CAmount>(-MAX_MONEY, MAX_MONEY);
    const int32_t other_virtual_bytes = fuzzed_data_provider.ConsumeIntegralInRange<int32_t>(-1, std::numeric_limits<int32_t>::max());
    AssertOrdering(paid, virtual_bytes, other_paid, other_virtual_bytes);
}
