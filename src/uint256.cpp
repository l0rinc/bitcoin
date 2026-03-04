// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <uint256.h>

#include <crypto/hex_base.h>
#include <util/strencodings.h>

#include <algorithm>
#include <optional>
#include <string_view>

namespace {
template <class uintN_t>
std::optional<uintN_t> ParseFixedHex(std::string_view str)
{
    if (uintN_t::size() * 2 != str.size()) return std::nullopt;
    uintN_t rv;
    unsigned char* p1 = rv.begin();
    unsigned char* pend = rv.end();
    size_t digits = str.size();
    while (digits > 0 && p1 < pend) {
        const signed char lo = ::HexDigit(str[--digits]);
        if (lo < 0) return std::nullopt;
        *p1 = static_cast<unsigned char>(lo);
        if (digits > 0) {
            const signed char hi = ::HexDigit(str[--digits]);
            if (hi < 0) return std::nullopt;
            *p1 |= static_cast<unsigned char>(hi << 4);
            p1++;
        }
    }
    return rv;
}

template <class uintN_t>
std::optional<uintN_t> ParseUserHex(std::string_view input)
{
    if (input.starts_with("0x")) input.remove_prefix(2);
    constexpr auto expected_size{uintN_t::size() * 2};
    if (input.size() < expected_size) {
        auto padded = std::string(expected_size, '0');
        std::copy(input.begin(), input.end(), padded.begin() + expected_size - input.size());
        return ParseFixedHex<uintN_t>(padded);
    }
    return ParseFixedHex<uintN_t>(input);
}
} // namespace

std::optional<uint160> uint160::FromHex(std::string_view str) { return ParseFixedHex<uint160>(str); }
std::optional<uint256> uint256::FromHex(std::string_view str) { return ParseFixedHex<uint256>(str); }
std::optional<uint256> uint256::FromUserHex(std::string_view str) { return ParseUserHex<uint256>(str); }

template <unsigned int BITS>
std::string base_blob<BITS>::GetHex() const
{
    uint8_t m_data_rev[WIDTH];
    for (int i = 0; i < WIDTH; ++i) {
        m_data_rev[i] = m_data[WIDTH - 1 - i];
    }
    return HexStr(m_data_rev);
}

template <unsigned int BITS>
std::string base_blob<BITS>::ToString() const
{
    return (GetHex());
}

// Explicit instantiations for base_blob<160>
template std::string base_blob<160>::GetHex() const;
template std::string base_blob<160>::ToString() const;

// Explicit instantiations for base_blob<256>
template std::string base_blob<256>::GetHex() const;
template std::string base_blob<256>::ToString() const;

const uint256 uint256::ZERO(0);
const uint256 uint256::ONE(1);
