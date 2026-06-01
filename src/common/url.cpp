// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/url.h>

#include <crypto/hex_base.h>

#include <string>
#include <string_view>

std::string UrlDecode(std::string_view url_encoded)
{
    std::string res;
    res.reserve(url_encoded.size());

    for (size_t i = 0; i < url_encoded.size(); ++i) {
        char c = url_encoded[i];
        // Special handling for percent which should be followed by two hex digits
        // representing an octet values, see RFC 3986, Section 2.1 Percent-Encoding
        if (c == '%' && i + 2 < url_encoded.size()) {
            const signed char high{HexDigit(url_encoded[i + 1])};
            const signed char low{HexDigit(url_encoded[i + 2])};

            // Only decode when both characters are valid hex.
            if (high >= 0 && low >= 0) {
                res += static_cast<char>((high << 4) | low);
                // Next two characters are part of the percent encoding
                i += 2;
                continue;
            }
            // In case of invalid percent encoding, add the '%' and continue
        }
        res += c;
    }

    return res;
}

std::string UrlEncode(std::string_view str)
{
    std::string res;
    res.reserve(str.size() * 3); // worst case: every char needs encoding

    for (char ch : str) {
        auto c = static_cast<unsigned char>(ch);
        // Unreserved characters per RFC 3986, Section 2.3
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            res += ch;
        } else {
            // Percent-encode all other characters
            res += '%';
            constexpr char hex_chars[] = "0123456789ABCDEF";
            res += hex_chars[(c >> 4) & 0xF];
            res += hex_chars[c & 0xF];
        }
    }
    return res;
}
