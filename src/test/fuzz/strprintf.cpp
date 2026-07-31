// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

template <typename... Args>
void fuzz_fmt(const std::string& fmt, const Args&... args)
{
    (void)tfm::format(tfm::RuntimeFormat{fmt}, args...);
}

std::string EscapePercentSigns(std::string_view input)
{
    std::string escaped;
    escaped.reserve(input.size() + std::ranges::count(input, '%'));
    for (const char ch : input) {
        if (ch == '%') escaped.push_back('%');
        escaped.push_back(ch);
    }
    return escaped;
}

struct FormatStringProperties {
    bool has_large_width_or_precision{false};
    bool has_positional_variable_width_or_precision{false};
    bool has_variable_width_or_precision{false};
    bool has_char_conversion{false};
};

FormatStringProperties InspectFormatString(std::string_view format_string)
{
    FormatStringProperties result;

    for (size_t pos{0}; pos < format_string.size();) {
        if (format_string[pos] != '%') {
            ++pos;
            continue;
        }
        if (++pos == format_string.size()) break;
        if (format_string[pos] == '%') {
            ++pos;
            continue;
        }

        bool positional_mode{false};
        bool initial_width_set{false};
        bool initial_digits_nonzero{false};
        const auto consume_digits = [&] {
            const size_t start{pos};
            while (pos < format_string.size() && IsDigit(format_string[pos])) {
                initial_digits_nonzero |= format_string[pos] != '0';
                ++pos;
            }
            return pos - start;
        };

        const size_t initial_digits{consume_digits()};
        if (initial_digits >= 7) result.has_large_width_or_precision = true;
        if (initial_digits != 0 && pos < format_string.size() && format_string[pos] == '$') {
            positional_mode = true;
            ++pos;
        } else {
            initial_width_set = initial_digits != 0 && initial_digits_nonzero;
        }

        const auto consume_width_or_precision = [&] {
            if (pos < format_string.size() && IsDigit(format_string[pos])) {
                const size_t digits{consume_digits()};
                if (digits >= 7) result.has_large_width_or_precision = true;
            } else if (pos < format_string.size() && format_string[pos] == '*') {
                result.has_variable_width_or_precision = true;
                ++pos;
                const size_t positional_digits{consume_digits()};
                if (positional_mode) {
                    result.has_positional_variable_width_or_precision = true;
                    if (positional_digits != 0 && pos < format_string.size() && format_string[pos] == '$') ++pos;
                }
            }
        };

        if (!initial_width_set) {
            while (pos < format_string.size() &&
                   (format_string[pos] == '#' || format_string[pos] == '0' ||
                    format_string[pos] == '-' || format_string[pos] == ' ' ||
                    format_string[pos] == '+')) {
                ++pos;
            }
            consume_width_or_precision();
        }
        if (pos < format_string.size() && format_string[pos] == '.') {
            ++pos;
            consume_width_or_precision();
        }
        while (pos < format_string.size() &&
               (format_string[pos] == 'l' || format_string[pos] == 'h' ||
                format_string[pos] == 'L' || format_string[pos] == 'j' ||
                format_string[pos] == 'z' || format_string[pos] == 't')) {
            ++pos;
        }
        if (pos < format_string.size()) {
            result.has_char_conversion |= format_string[pos] == 'c';
            ++pos;
        }
    }

    return result;
}

FUZZ_TARGET(str_printf)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string format_string = fuzzed_data_provider.ConsumeRandomLengthString(64);

    if (util::ContainsNoNUL(format_string)) {
        assert(tfm::format(tfm::RuntimeFormat{EscapePercentSigns(format_string)}) == format_string);
    }

    const FormatStringProperties format_properties{InspectFormatString(format_string)};

    // Avoid triggering the following crash bug:
    // * strprintf("%987654321000000:", 1);
    //
    // Avoid triggering the following OOM bug:
    // * strprintf("%.222222200000000$", 1.1);
    //
    // Also avoid the following crash bug:
    // * strprintf("%1$*1$*", -11111111);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70
    if (format_properties.has_large_width_or_precision || format_properties.has_positional_variable_width_or_precision) {
        return;
    }

    // Avoid triggering the following crash bug:
    // * strprintf("%.1s", (char*)nullptr);
    //
    // (void)strprintf(format_string, (char*)nullptr);
    //
    // Upstream bug report: https://github.com/c42f/tinyformat/issues/70

    try {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeRandomLengthString(32));
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeRandomLengthString(32).c_str());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<signed char>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<unsigned char>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<char>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeBool());
            });
    } catch (const tinyformat::format_error&) {
    }

    if (format_properties.has_char_conversion) {
        // Avoid triggering the following:
        // * strprintf("%c", 1.31783e+38);
        // tinyformat.h:244:36: runtime error: 1.31783e+38 is outside the range of representable values of type 'char'
        return;
    }

    if (format_properties.has_variable_width_or_precision) {
        // Avoid triggering the following:
        // * strprintf("%*", -2.33527e+38);
        // tinyformat.h:283:65: runtime error: -2.33527e+38 is outside the range of representable values of type 'int'
        // * strprintf("%*", -2147483648);
        // tinyformat.h:763:25: runtime error: negation of -2147483648 cannot be represented in type 'int'; cast to an unsigned type to negate this value to itself
        return;
    }

    try {
        CallOneOf(
            fuzzed_data_provider,
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeFloatingPoint<float>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeFloatingPoint<double>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<int16_t>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<uint16_t>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<int32_t>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<uint32_t>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<int64_t>());
            },
            [&] {
                fuzz_fmt(format_string, fuzzed_data_provider.ConsumeIntegral<uint64_t>());
            });
    } catch (const tinyformat::format_error&) {
    }
}
