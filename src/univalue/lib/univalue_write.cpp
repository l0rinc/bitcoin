// Copyright 2014 BitPay Inc.
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#include <univalue.h>
#include <univalue_escapes.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#ifndef NDEBUG
static bool IsHexDigit(char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

static bool IsJsonEscapedStringFragment(std::string_view str)
{
    for (size_t i = 0; i < str.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(str[i]);
        if (ch < 0x20 || ch == 0x7f || ch == '"') return false;
        if (ch != '\\') continue;

        if (++i == str.size()) return false;
        switch (str[i]) {
        case '"':
        case '\\':
        case 'b':
        case 't':
        case 'n':
        case 'f':
        case 'r':
            break;
        case 'u':
            if (i + 4 >= str.size()) return false;
            for (size_t j = 1; j <= 4; ++j) {
                if (!IsHexDigit(str[i + j])) return false;
            }
            i += 4;
            break;
        default:
            return false;
        }
    }
    return true;
}
#endif

static std::string json_escape(const std::string& inS)
{
    std::string outS;
    outS.reserve(inS.size() * 2);

    for (unsigned int i = 0; i < inS.size(); i++) {
        unsigned char ch = static_cast<unsigned char>(inS[i]);
        const char *escStr = escapes[ch];

        if (escStr)
            outS += escStr;
        else
            outS += static_cast<char>(ch);
    }

    assert(IsJsonEscapedStringFragment(outS));
    return outS;
}

// NOLINTNEXTLINE(misc-no-recursion)
std::string UniValue::write(unsigned int prettyIndent,
                            unsigned int indentLevel) const
{
    std::string s;
    s.reserve(1024);

    unsigned int modIndent = indentLevel;
    if (modIndent == 0)
        modIndent = 1;

    switch (typ) {
    case VNULL:
        s += "null";
        break;
    case VOBJ:
        writeObject(prettyIndent, modIndent, s);
        break;
    case VARR:
        writeArray(prettyIndent, modIndent, s);
        break;
    case VSTR:
        s += "\"" + json_escape(val) + "\"";
        break;
    case VNUM:
        s += val;
        break;
    case VBOOL:
        s += (val == "1" ? "true" : "false");
        break;
    }

    return s;
}

static void indentStr(unsigned int prettyIndent, unsigned int indentLevel, std::string& s)
{
    s.append(prettyIndent * indentLevel, ' ');
}

// NOLINTNEXTLINE(misc-no-recursion)
void UniValue::writeArray(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const
{
    assert(typ == VARR);
    s += "[";
    if (prettyIndent)
        s += "\n";

    for (unsigned int i = 0; i < values.size(); i++) {
        if (prettyIndent)
            indentStr(prettyIndent, indentLevel, s);
        s += values[i].write(prettyIndent, indentLevel + 1);
        if (i != (values.size() - 1)) {
            s += ",";
        }
        if (prettyIndent)
            s += "\n";
    }

    if (prettyIndent)
        indentStr(prettyIndent, indentLevel - 1, s);
    s += "]";
}

// NOLINTNEXTLINE(misc-no-recursion)
void UniValue::writeObject(unsigned int prettyIndent, unsigned int indentLevel, std::string& s) const
{
    assert(typ == VOBJ);
    assert(keys.size() == values.size());
    s += "{";
    if (prettyIndent)
        s += "\n";

    for (unsigned int i = 0; i < keys.size(); i++) {
        if (prettyIndent)
            indentStr(prettyIndent, indentLevel, s);
        s += "\"" + json_escape(keys[i]) + "\":";
        if (prettyIndent)
            s += " ";
        s += values.at(i).write(prettyIndent, indentLevel + 1);
        if (i != (values.size() - 1))
            s += ",";
        if (prettyIndent)
            s += "\n";
    }

    if (prettyIndent)
        indentStr(prettyIndent, indentLevel - 1, s);
    s += "}";
}
