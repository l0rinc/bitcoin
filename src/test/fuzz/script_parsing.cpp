// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/parsing.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <util/byte_units.h>
#include <util/string.h>

#include <cassert>
#include <cstddef>
#include <span>
#include <string_view>

using util::Split;

namespace {

bool SpanIsSame(const std::span<const char>& a, const std::span<const char>& b)
{
    return a.data() == b.data() && a.size() == b.size();
}

size_t ExpectedExprSize(std::string_view sp)
{
    int level{0};
    for (size_t pos{0}; pos < sp.size(); ++pos) {
        const char ch{sp[pos]};
        if (ch == '(' || ch == '{') {
            ++level;
        } else if (level && (ch == ')' || ch == '}')) {
            --level;
        } else if (level == 0 && (ch == ')' || ch == '}' || ch == ',')) {
            return pos;
        }
    }
    return sp.size();
}

} // namespace

FUZZ_TARGET(script_parsing)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const size_t query_size = fuzzed_data_provider.ConsumeIntegral<size_t>();
    const std::string query = fuzzed_data_provider.ConsumeBytesAsString(std::min<size_t>(query_size, 1_MiB));
    const std::string span_str = fuzzed_data_provider.ConsumeRemainingBytesAsString();
    const std::span<const char> const_span{span_str};
    const std::string_view query_view{query.data(), query.size()};
    const std::string_view span_view{span_str.data(), span_str.size()};

    std::span<const char> mut_span = const_span;
    const bool expect_const{span_view.size() >= query_view.size() && span_view.substr(0, query_view.size()) == query_view};
    assert(script::Const(query, mut_span) == expect_const);
    assert(SpanIsSame(mut_span, expect_const ? const_span.subspan(query.size()) : const_span));

    mut_span = const_span;
    assert(script::Const(query, mut_span, /*skip=*/false) == expect_const);
    assert(SpanIsSame(mut_span, const_span));

    mut_span = const_span;
    const bool expect_func{
        span_view.size() >= query_view.size() + 2 &&
        span_view.substr(0, query_view.size()) == query_view &&
        span_view[query_view.size()] == '(' &&
        span_view.back() == ')'};
    assert(script::Func(query, mut_span) == expect_func);
    assert(SpanIsSame(mut_span, expect_func ? const_span.subspan(query.size() + 1, span_view.size() - query_view.size() - 2) : const_span));

    mut_span = const_span;
    const size_t expected_expr_size{ExpectedExprSize(span_view)};
    const std::span<const char> expr{script::Expr(mut_span)};
    assert(SpanIsSame(expr, const_span.first(expected_expr_size)));
    assert(SpanIsSame(mut_span, const_span.subspan(expected_expr_size)));
    if (!mut_span.empty()) {
        assert(mut_span.front() == ')' || mut_span.front() == '}' || mut_span.front() == ',');
    }

    if (!query.empty()) {
        mut_span = const_span;
        (void)Split(mut_span, query.front());
    }
}
