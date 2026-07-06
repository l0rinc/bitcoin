// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/amount.h>
#include <policy/feerate.h>
#include <tinyformat.h>

namespace {
struct FeeRateParts {
    bool negative;
    CAmount quotient;
    CAmount remainder;
};

FeeRateParts SplitFeeRate(const CAmount feerate_per_kvb, const CAmount scale)
{
    Assume(scale > 1);

    FeeRateParts parts{
        .negative = feerate_per_kvb < 0,
        .quotient = feerate_per_kvb / scale,
        .remainder = feerate_per_kvb % scale,
    };
    if (parts.negative) {
        parts.quotient = -parts.quotient;
        parts.remainder = -parts.remainder;
    }

    Assume(parts.quotient >= 0);
    Assume(parts.remainder >= 0);
    Assume(parts.remainder < scale);
    return parts;
}

void AddSign(const bool negative, std::string& str)
{
    if (negative) str.insert(0, 1, '-');
}
} // namespace


CFeeRate::CFeeRate(const CAmount& nFeePaid, int32_t virtual_bytes)
{
    if (virtual_bytes > 0) {
        m_feerate = FeePerVSize(nFeePaid, virtual_bytes);
        Assume(GetFee(virtual_bytes) == nFeePaid);
    } else {
        m_feerate = FeePerVSize();
    }
}

CAmount CFeeRate::GetFee(int32_t virtual_bytes) const
{
    Assume(virtual_bytes >= 0);
    if (m_feerate.IsEmpty()) { return CAmount(0);}
    CAmount nFee = CAmount(m_feerate.EvaluateFeeUp(virtual_bytes));
    if (nFee == 0 && virtual_bytes != 0 && m_feerate.fee < 0) return CAmount(-1);
    return nFee;
}

std::string CFeeRate::ToString(FeeRateFormat fee_rate_format) const
{
    const CAmount feerate_per_kvb{GetFeePerK()};
    switch (fee_rate_format) {
    case FeeRateFormat::BTC_KVB: {
        const FeeRateParts parts{SplitFeeRate(feerate_per_kvb, COIN)};
        auto str{strprintf("%d.%08d %s/kvB", parts.quotient, parts.remainder, CURRENCY_UNIT)};
        AddSign(parts.negative, str);
        return str;
    }
    case FeeRateFormat::SAT_VB: {
        constexpr CAmount SATS_PER_VB{1000};
        const FeeRateParts parts{SplitFeeRate(feerate_per_kvb, SATS_PER_VB)};
        auto str{strprintf("%d.%03d %s/vB", parts.quotient, parts.remainder, CURRENCY_ATOM)};
        AddSign(parts.negative, str);
        return str;
    }
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}
