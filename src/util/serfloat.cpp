// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/serfloat.h>

#include <util/check.h>

#include <cmath>
#include <limits>

namespace {

constexpr uint64_t DOUBLE_SIGN_MASK{0x8000000000000000};
constexpr uint64_t DOUBLE_EXP_MASK{0x7FF0000000000000};
constexpr uint64_t DOUBLE_MAN_MASK{0x000FFFFFFFFFFFFF};
constexpr uint64_t DOUBLE_INF{0x7ff0000000000000};
constexpr uint64_t DOUBLE_CANONICAL_NAN{0x7ff8000000000000};

} // namespace

double DecodeDouble(uint64_t v) noexcept {
    static constexpr double NANVAL = std::numeric_limits<double>::quiet_NaN();
    static constexpr double INFVAL = std::numeric_limits<double>::infinity();
    double sign = 1.0;
    if (v & DOUBLE_SIGN_MASK) {
        sign = -1.0;
        v ^= DOUBLE_SIGN_MASK;
    }
    // Zero
    if (v == 0) return copysign(0.0, sign);
    // Infinity
    if (v == DOUBLE_INF) return copysign(INFVAL, sign);
    // Other numbers
    int exp = (v & DOUBLE_EXP_MASK) >> 52;
    uint64_t man = v & DOUBLE_MAN_MASK;
    if (exp == 2047) {
        // NaN
        Assume(man != 0);
        return NANVAL;
    } else if (exp == 0) {
        // Subnormal
        return copysign(ldexp((double)man, -1074), sign);
    } else {
        // Normal
        return copysign(ldexp((double)(man + 0x10000000000000), -1075 + exp), sign);
    }
}

uint64_t EncodeDouble(double f) noexcept {
    int cls = std::fpclassify(f);
    uint64_t sign = 0;
    if (copysign(1.0, f) == -1.0) {
        f = -f;
        sign = 0x8000000000000000;
    }
    // Zero
    if (cls == FP_ZERO) return sign;
    // Infinity
    if (cls == FP_INFINITE) return sign | DOUBLE_INF;
    // NaN
    if (cls == FP_NAN) return DOUBLE_CANONICAL_NAN;
    // Other numbers
    int exp;
    uint64_t man = std::round(std::frexp(f, &exp) * 9007199254740992.0);
    if (exp < -1021) {
        // Too small to represent, encode 0
        if (exp < -1084) return sign;
        // Subnormal numbers
        return sign | (man >> (-1021 - exp));
    } else {
        // Too big to represent, encode infinity
        if (exp > 1024) return sign | DOUBLE_INF;
        // Normal numbers
        return sign | (((uint64_t)(1022 + exp)) << 52) | (man & DOUBLE_MAN_MASK);
    }
}
