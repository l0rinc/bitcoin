// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#ifdef WIN32
#include <util/syserror.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <limits>
#include <stdexcept>
#endif

#include <regex>
#include <string>

namespace util {
void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute)
{
    if (search.empty()) return;
    in_out = std::regex_replace(in_out, std::regex(search), substitute);
}

#ifdef WIN32
std::wstring Utf8ToWide(std::string_view utf8)
{
    if (utf8.empty()) return {};
    if (utf8.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("UTF-8 string is too long to convert");
    }

    const int src_size{static_cast<int>(utf8.size())};
    const int dst_size{MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), src_size, nullptr, 0)};
    if (dst_size == 0) {
        const auto err{GetLastError()};
        throw std::runtime_error("MultiByteToWideChar failed: " + Win32ErrorString(err));
    }

    std::wstring wide(dst_size, 0);
    const int result{MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), src_size, wide.data(), dst_size)};
    if (result != dst_size) {
        const auto err{GetLastError()};
        throw std::runtime_error("MultiByteToWideChar failed: " + Win32ErrorString(err));
    }

    return wide;
}
#endif
} // namespace util
