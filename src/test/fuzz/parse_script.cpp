// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <script/script.h>
#include <test/fuzz/fuzz.h>
#include <util/strencodings.h>

#include <algorithm>
#include <charconv>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

std::vector<std::string_view> Tokenize(const std::string_view input)
{
    std::vector<std::string_view> tokens;
    size_t start{0};
    while (start < input.size()) {
        while (start < input.size() && (input[start] == ' ' || input[start] == '\t' || input[start] == '\n')) {
            ++start;
        }
        if (start == input.size()) break;
        size_t end{start};
        while (end < input.size() && input[end] != ' ' && input[end] != '\t' && input[end] != '\n') {
            ++end;
        }
        tokens.emplace_back(input.substr(start, end - start));
        start = end;
    }
    return tokens;
}

std::string Normalize(const std::vector<std::string_view>& tokens)
{
    std::string normalized;
    for (const auto token : tokens) {
        if (!normalized.empty()) normalized.push_back(' ');
        normalized.append(token);
    }
    return normalized;
}

std::optional<int64_t> ParseDecimal(const std::string_view token)
{
    const bool decimal{!token.empty() &&
                       (std::all_of(token.begin(), token.end(), IsDigit) ||
                        (token.front() == '-' && token.size() > 1 && std::all_of(token.begin() + 1, token.end(), IsDigit)))};
    if (!decimal) return std::nullopt;

    int64_t value{0};
    const auto [end, error]{std::from_chars(token.data(), token.data() + token.size(), value)};
    if (error != std::errc{} || end != token.data() + token.size() || value > 0xffffffff || value < -int64_t{0xffffffff}) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::vector<unsigned char>> ParseRawHex(const std::string_view token)
{
    if (!token.starts_with("0x") || token.size() <= 2 || token.size() % 2 != 0) return std::nullopt;
    std::vector<unsigned char> bytes;
    bytes.reserve((token.size() - 2) / 2);
    for (size_t i{2}; i < token.size(); i += 2) {
        const int high{HexDigit(token[i])};
        const int low{HexDigit(token[i + 1])};
        if (high < 0 || low < 0) return std::nullopt;
        bytes.push_back(static_cast<unsigned char>((high << 4) | low));
    }
    return bytes;
}

std::optional<opcodetype> ParseOpcode(const std::string_view token)
{
    for (unsigned int opcode{0}; opcode <= MAX_OPCODE; ++opcode) {
        const std::string name{GetOpName(static_cast<opcodetype>(opcode))};
        if (token == name || (name.starts_with("OP_") && token == std::string_view{name}.substr(3))) {
            return static_cast<opcodetype>(opcode);
        }
    }
    return std::nullopt;
}

CScript ExpectedToken(const std::string_view token)
{
    CScript expected;
    if (const auto value{ParseDecimal(token)}) {
        expected << *value;
    } else if (const auto raw{ParseRawHex(token)}) {
        expected.insert(expected.end(), raw->begin(), raw->end());
    } else if (token.size() >= 2 && token.front() == '\'' && token.back() == '\'') {
        expected << std::span<const unsigned char>{reinterpret_cast<const unsigned char*>(token.data() + 1), token.size() - 2};
    } else if (const auto opcode{ParseOpcode(token)}) {
        expected << *opcode;
    } else {
        throw std::runtime_error{"token is not accepted by the independent parser"};
    }
    return expected;
}

void AssertParseScriptContracts(const std::string_view input)
{
    const auto tokens{Tokenize(input)};
    CScript parsed;
    try {
        parsed = ParseScript(std::string{input});
    } catch (const std::runtime_error&) {
        return;
    }

    const std::string normalized{Normalize(tokens)};
    assert(ParseScript(normalized) == parsed);
    assert(parsed.empty() == tokens.empty());

    CScript expected;
    for (const auto token : tokens) {
        const CScript expected_token{ExpectedToken(token)};
        expected.insert(expected.end(), expected_token.begin(), expected_token.end());
    }
    assert(expected == parsed);

    // Avoid quadratic allocator pressure on pathological, one-byte inputs while
    // still checking state isolation for the common tokenized-input domain.
    if (tokens.size() <= 512) {
        CScript decomposed;
        for (const auto token : tokens) {
            const CScript parsed_token{ParseScript(std::string{token})};
            decomposed.insert(decomposed.end(), parsed_token.begin(), parsed_token.end());
        }
        assert(decomposed == parsed);
    }
}

} // namespace

FUZZ_TARGET(parse_script)
{
    AssertParseScriptContracts(std::string_view{reinterpret_cast<const char*>(buffer.data()), buffer.size()});
}
