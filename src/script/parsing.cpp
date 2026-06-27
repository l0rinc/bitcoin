// Copyright (c) 2018-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/parsing.h>

#include <util/check.h>

#include <algorithm>
#include <cstddef>
#include <string>

namespace {

bool SpanIsSame(const std::span<const char>& a, const std::span<const char>& b)
{
    return a.data() == b.data() && a.size() == b.size();
}

bool SpanIsSubspan(const std::span<const char>& original, const std::span<const char>& subspan, size_t offset, size_t size)
{
    if (offset > original.size() || size > original.size() - offset || subspan.size() != size) return false;
    if (original.empty()) return subspan.empty();
    return subspan.data() == original.data() + offset;
}

bool SpanIsPrefix(const std::span<const char>& original, const std::span<const char>& prefix)
{
    return SpanIsSubspan(original, prefix, /*offset=*/0, prefix.size());
}

bool SpanIsSuffix(const std::span<const char>& original, const std::span<const char>& suffix, size_t offset)
{
    return offset <= original.size() && SpanIsSubspan(original, suffix, offset, original.size() - offset);
}

} // namespace

namespace script {

bool Const(const std::string& str, std::span<const char>& sp, bool skip)
{
    const auto original{sp};
    if ((size_t)sp.size() >= str.size() && std::equal(str.begin(), str.end(), sp.begin())) {
        if (skip) sp = sp.subspan(str.size());
        Assume(skip ? SpanIsSuffix(original, sp, str.size()) : SpanIsSame(original, sp));
        return true;
    }
    Assume(SpanIsSame(original, sp));
    return false;
}

bool Func(const std::string& str, std::span<const char>& sp)
{
    const auto original{sp};
    if ((size_t)sp.size() >= str.size() + 2 && sp[str.size()] == '(' && sp[sp.size() - 1] == ')' && std::equal(str.begin(), str.end(), sp.begin())) {
        sp = sp.subspan(str.size() + 1, sp.size() - str.size() - 2);
        Assume(SpanIsSubspan(original, sp, str.size() + 1, original.size() - str.size() - 2));
        return true;
    }
    Assume(SpanIsSame(original, sp));
    return false;
}

std::span<const char> Expr(std::span<const char>& sp)
{
    const auto original{sp};
    int level = 0;
    auto it = sp.begin();
    while (it != sp.end()) {
        if (*it == '(' || *it == '{') {
            ++level;
        } else if (level && (*it == ')' || *it == '}')) {
            --level;
        } else if (level == 0 && (*it == ')' || *it == '}' || *it == ',')) {
            break;
        }
        ++it;
    }
    std::span<const char> ret = sp.first(it - sp.begin());
    sp = sp.subspan(it - sp.begin());
    Assume(SpanIsPrefix(original, ret));
    Assume(SpanIsSuffix(original, sp, ret.size()));
    return ret;
}

} // namespace script
