// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/license_info.h>
#include <common/messages.h>
#include <common/settings.h>
#include <common/system.h>
#include <common/url.h>
#include <netbase.h>
#include <outputtype.h>
#include <rpc/client.h>
#include <rpc/request.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <serialize.h>
#include <streams.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/fees.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/translation.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <ios>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using common::AmountErrMsg;
using common::AmountHighWarn;
using common::FeeModeFromString;
using common::ResolveErrMsg;
using util::ContainsNoNUL;
using util::Join;
using util::LineReader;
using util::RemovePrefix;
using util::RemovePrefixView;
using util::RemoveSuffixView;
using util::Split;
using util::SplitString;
using util::TrimString;

namespace {
void AssertAffixRemovalContracts(std::string_view str, std::string_view affix)
{
    const std::string_view removed_prefix_view{RemovePrefixView(str, affix)};
    const std::string removed_prefix{RemovePrefix(str, affix)};
    assert(removed_prefix == removed_prefix_view);
    if (str.starts_with(affix)) {
        assert(removed_prefix_view == str.substr(affix.size()));
        assert(affix.size() + removed_prefix_view.size() == str.size());
    } else {
        assert(removed_prefix_view == str);
    }

    const std::string_view removed_suffix_view{RemoveSuffixView(str, affix)};
    if (str.ends_with(affix)) {
        assert(removed_suffix_view == str.substr(0, str.size() - affix.size()));
        assert(removed_suffix_view.size() + affix.size() == str.size());
    } else {
        assert(removed_suffix_view == str);
    }
}
} // namespace

FUZZ_TARGET(string)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    const std::string random_string_1 = fuzzed_data_provider.ConsumeRandomLengthString(32);
    const std::string random_string_2 = fuzzed_data_provider.ConsumeRandomLengthString(32);
    const std::vector<std::string> random_string_vector = ConsumeRandomLengthStringVector(fuzzed_data_provider);

    (void)AmountErrMsg(random_string_1, random_string_2);
    (void)AmountHighWarn(random_string_1);
    BlockFilterType block_filter_type;
    (void)BlockFilterTypeByName(random_string_1, block_filter_type);
    (void)Capitalize(random_string_1);
    (void)CopyrightHolders(random_string_1);
    FeeEstimateMode fee_estimate_mode;
    (void)FeeModeFromString(random_string_1, fee_estimate_mode);
    const auto width{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(1, 1000)};
    (void)FormatParagraph(random_string_1, width, fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, width));
    (void)FormatSubVersion(random_string_1, fuzzed_data_provider.ConsumeIntegral<int>(), random_string_vector);
    (void)HelpExampleCli(random_string_1, random_string_2);
    (void)HelpExampleRpc(random_string_1, random_string_2);
    (void)HelpMessageGroup(random_string_1);
    (void)HelpMessageOpt(random_string_1, "", random_string_2);
    (void)HelpMessageOpt(random_string_1, random_string_2, "");
    (void)IsDeprecatedRPCEnabled(random_string_1);
    (void)Join(random_string_vector, random_string_1);
    (void)JSONRPCError(fuzzed_data_provider.ConsumeIntegral<int>(), random_string_1);
    const common::Settings settings;
    (void)OnlyHasDefaultSectionSetting(settings, random_string_1, random_string_2);
    (void)ParseNetwork(random_string_1);
    (void)ParseOutputType(random_string_1);
    AssertAffixRemovalContracts(random_string_1, random_string_2);
    (void)ResolveErrMsg(random_string_1, random_string_2);
    try {
        (void)RPCConvertNamedValues(random_string_1, random_string_vector);
    } catch (const RPCConvertError&) {
    }
    try {
        (void)RPCConvertValues(random_string_1, random_string_vector);
    } catch (const RPCConvertError&) {
    }
    (void)SanitizeString(random_string_1);
    (void)SanitizeString(random_string_1, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 3));
#ifndef WIN32
    (void)ShellEscape(random_string_1);
#endif // WIN32
    uint16_t port_out;
    std::string host_out;
    SplitHostPort(random_string_1, port_out, host_out);
    (void)TimingResistantEqual(random_string_1, random_string_2);
    (void)ToLower(random_string_1);
    (void)ToUpper(random_string_1);
    (void)TrimString(random_string_1);
    (void)TrimString(random_string_1, random_string_2);
    (void)UrlDecode(random_string_1);
    (void)ContainsNoNUL(random_string_1);
    try {
        throw scriptnum_error{random_string_1};
    } catch (const scriptnum_error&) {
    }

    {
        DataStream data_stream{};
        std::string s;
        auto limited_string = LIMITED_STRING(s, 10);
        data_stream << random_string_1;
        try {
            data_stream >> limited_string;
            assert(data_stream.empty());
            assert(s.size() <= random_string_1.size());
            assert(s.size() <= 10);
            if (!random_string_1.empty()) {
                assert(!s.empty());
            }
        } catch (const std::ios_base::failure&) {
        }
    }
    {
        DataStream data_stream{};
        const auto limited_string = LIMITED_STRING(random_string_1, 10);
        data_stream << limited_string;
        std::string deserialized_string;
        data_stream >> deserialized_string;
        assert(data_stream.empty());
        assert(deserialized_string == random_string_1);
    }
    {
        int64_t amount_out;
        (void)ParseFixedPoint(random_string_1, fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 1024), &amount_out);
    }
    {
        const char separator{fuzzed_data_provider.ConsumeIntegral<char>()};
        const std::string separator_string{separator};
        const auto single_split{SplitString(random_string_1, separator)};
        const size_t separator_count{static_cast<size_t>(std::count(random_string_1.begin(), random_string_1.end(), separator))};
        assert(single_split.size() >= 1);
        assert(single_split.size() == separator_count + 1);
        assert(Join(single_split, separator_string) == random_string_1);

        const auto included_separator_split{Split<std::string>(random_string_1, separator, /*include_sep=*/true)};
        assert(included_separator_split.size() == single_split.size());
        assert(Join(included_separator_split, std::string{}) == random_string_1);
        for (size_t i{0}; i + 1 < included_separator_split.size(); ++i) {
            assert(!included_separator_split.at(i).empty());
            assert(included_separator_split.at(i).back() == separator);
        }

        const auto any_split{SplitString(random_string_1, random_string_2)};
        assert(any_split.size() >= 1);
    }
    {
        const std::string line_buffer{fuzzed_data_provider.ConsumeRandomLengthString(128)};
        const size_t max_line_length{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, 64)};
        LineReader reader{line_buffer, max_line_length};
        const std::string_view buffer_view{line_buffer};

        const auto newline_pos{buffer_view.find('\n')};
        const bool read_line_should_throw{newline_pos == std::string_view::npos ?
            buffer_view.size() > max_line_length :
            newline_pos > max_line_length};

        try {
            const std::optional<std::string_view> line{reader.ReadLine()};
            assert(!read_line_should_throw);
            if (newline_pos == std::string_view::npos) {
                assert(!line);
                assert(reader.Consumed() == 0);
                assert(reader.Remaining() == buffer_view.size());
            } else {
                std::string_view expected_line{buffer_view.substr(0, newline_pos)};
                expected_line = util::RemoveSuffixView(expected_line, "\r");
                assert(line.has_value());
                assert(*line == expected_line);
                assert(reader.Consumed() == newline_pos + 1);
                assert(reader.Remaining() == buffer_view.size() - reader.Consumed());
            }
        } catch (const std::runtime_error& e) {
            assert(std::string_view{e.what()} == "max_line_length exceeded by LineReader");
            assert(read_line_should_throw);
            assert(reader.Consumed() == 0);
            assert(reader.Remaining() == buffer_view.size());
        }

        const size_t consumed_before{reader.Consumed()};
        const size_t remaining_before{reader.Remaining()};
        const size_t read_length{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, buffer_view.size() + 2)};
        try {
            const std::string_view chunk{reader.ReadLength(read_length)};
            assert(read_length <= remaining_before);
            assert(chunk == buffer_view.substr(consumed_before, read_length));
            assert(reader.Consumed() == consumed_before + read_length);
            assert(reader.Remaining() == remaining_before - read_length);
        } catch (const std::runtime_error& e) {
            assert(std::string_view{e.what()} == "Not enough data in buffer");
            assert(read_length > remaining_before);
            assert(reader.Consumed() == consumed_before);
            assert(reader.Remaining() == remaining_before);
        }
        assert(reader.Consumed() + reader.Remaining() == buffer_view.size());
    }
    {
        (void)Untranslated(random_string_1);
        const bilingual_str bs1{random_string_1, random_string_2};
        const bilingual_str bs2{random_string_2, random_string_1};
        (void)(bs1 + bs2);
    }
    {
        const ByteUnit all_units[] = {
            ByteUnit::NOOP,
            ByteUnit::k,
            ByteUnit::K,
            ByteUnit::m,
            ByteUnit::M,
            ByteUnit::g,
            ByteUnit::G,
            ByteUnit::t,
            ByteUnit::T
        };
        ByteUnit default_multiplier = fuzzed_data_provider.PickValueInArray(all_units);
        (void)ParseByteUnits(random_string_1, default_multiplier);
    }
}
