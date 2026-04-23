// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_TEST_UTIL_CHECK_H
#define BITCOIN_TEST_UTIL_CHECK_H

#include <util/check.h>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <exception>
#include <functional>
#include <iterator>
#include <optional>
#include <ostream>
#include <ranges>
#include <sstream>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

class HasReason
{
public:
    explicit HasReason(std::string_view reason) : m_reason(reason) {}
    bool operator()(std::string_view s) const { return s.find(m_reason) != std::string_view::npos; }
    bool operator()(const std::exception& e) const { return (*this)(e.what()); }

private:
    const std::string m_reason;
};

namespace std {
template <typename Clock, typename Duration>
inline std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<Clock, Duration>& tp)
{
    return os << tp.time_since_epoch().count();
}

template <typename T> requires std::is_enum_v<T>
inline std::ostream& operator<<(std::ostream& os, const T& e)
{
    return os << static_cast<std::underlying_type_t<T>>(e);
}

template <typename T> requires requires(std::ostream& os, const T& t) { os << t; }
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    return v ? os << *v
             : os << "std::nullopt";
}
} // namespace std

template <typename T>
concept HasToString = requires(const T& t) { t.ToString(); };

template <HasToString T>
inline std::ostream& operator<<(std::ostream& os, const T& obj)
{
    return os << obj.ToString();
}

namespace test::check_detail {

template <typename T>
using Decay = std::remove_cvref_t<T>;

template <typename T>
inline constexpr bool IsBool{std::same_as<Decay<T>, bool>};

template <typename T>
inline constexpr bool IsIntegralNoBool{std::is_integral_v<Decay<T>> && !IsBool<T>};

template <typename T>
inline constexpr bool IsRawArray{std::is_array_v<Decay<T>>};

template <typename T>
inline constexpr bool IsRawPointer{std::is_pointer_v<Decay<T>>};

template <typename T>
inline constexpr bool IsChar{std::same_as<std::remove_cv_t<Decay<T>>, char>};

template <typename T>
inline constexpr bool IsCharArray{IsRawArray<T> && IsChar<std::remove_extent_t<Decay<T>>>};

template <typename T>
inline constexpr bool IsCharPointer{IsRawPointer<T> && IsChar<std::remove_pointer_t<Decay<T>>>};

template <typename L, typename R>
inline constexpr bool SameIntegralSignedness{
    !(IsIntegralNoBool<L> && IsIntegralNoBool<R>) ||
    (std::is_signed_v<Decay<L>> == std::is_signed_v<Decay<R>>)};

template <typename L, typename R>
inline constexpr bool SameBoolness{IsBool<L> == IsBool<R>};

template <typename L, typename R>
concept ValidComparisonTypes =
    !IsRawArray<L> && !IsRawArray<R> &&
    !IsRawPointer<L> && !IsRawPointer<R> &&
    SameBoolness<L, R> &&
    SameIntegralSignedness<L, R>;

inline void AppendHexByte(std::string& out, uint8_t byte)
{
    constexpr char hexmap[]{"0123456789abcdef"};
    out += "\\x";
    out += hexmap[byte >> 4];
    out += hexmap[byte & 0x0f];
}

inline std::string EscapeString(std::string_view str)
{
    std::string out;
    out.reserve(str.size() + 2);
    out += '"';
    for (const unsigned char c : str) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c >= 0x20 && c <= 0x7e) {
                out += static_cast<char>(c);
            } else {
                AppendHexByte(out, c);
            }
        }
    }
    out += '"';
    return out;
}

template <typename T>
inline constexpr bool IsByteValue{
    std::same_as<Decay<T>, std::byte> ||
    std::same_as<Decay<T>, unsigned char> ||
    std::same_as<Decay<T>, signed char> ||
    std::same_as<Decay<T>, uint8_t> ||
    std::same_as<Decay<T>, int8_t>};

template <typename T>
inline constexpr bool IsOptional{false};

template <typename T>
inline constexpr bool IsOptional<std::optional<T>>{true};

template <typename T>
concept StringValue = std::same_as<Decay<T>, std::string> || std::same_as<Decay<T>, std::string_view>;

template <typename T>
concept Streamable = requires(std::ostream& os, const T& value) {
    os << value;
};

template <typename T>
concept ConstRange = std::ranges::input_range<const Decay<T>&>;

template <typename T>
using RangeValue = std::ranges::range_value_t<const Decay<T>&>;

template <typename T>
concept CharRange = ConstRange<T> && IsChar<RangeValue<T>>;

template <typename T>
concept ByteRange = ConstRange<T> && IsByteValue<RangeValue<T>> && !CharRange<T>;

template <typename T>
concept GenericRange = ConstRange<T> && !StringValue<T> && !CharRange<T> && !ByteRange<T>;

template <typename T>
std::string FormatValue(const T& value);

template <typename Fn>
std::string FormatMessage(Fn&& fn)
{
    try {
        return std::invoke(std::forward<Fn>(fn));
    } catch (...) {
        return "<failed to format message>";
    }
}

template <typename Fn>
std::string StreamMessage(Fn&& fn)
{
    return FormatMessage([&] {
        std::ostringstream oss;
        std::invoke(std::forward<Fn>(fn), oss);
        return oss.str();
    });
}

inline std::string FormatByte(uint8_t byte)
{
    constexpr char hexmap[]{"0123456789abcdef"};
    std::string out{"0x"};
    out += hexmap[byte >> 4];
    out += hexmap[byte & 0x0f];
    return out;
}

template <typename T>
uint8_t ByteValue(const T& value)
{
    if constexpr (std::same_as<Decay<T>, std::byte>) {
        return std::to_integer<uint8_t>(value);
    } else {
        return static_cast<uint8_t>(value);
    }
}

template <typename T>
std::string FormatCharArray(const T& value)
{
    size_t size{std::extent_v<Decay<T>>};
    if (size > 0 && value[size - 1] == '\0') --size;
    return EscapeString({value, size});
}

template <typename T>
std::string FormatByteRange(const T& value)
{
    std::string out{"hex\""};
    constexpr char hexmap[]{"0123456789abcdef"};
    for (const auto& elem : value) {
        const uint8_t byte{ByteValue(elem)};
        out += hexmap[byte >> 4];
        out += hexmap[byte & 0x0f];
    }
    out += '"';
    return out;
}

template <typename T>
std::string FormatCharRange(const T& value)
{
    std::string out;
    for (const auto& elem : value) {
        out += elem;
    }
    return EscapeString(out);
}

template <typename T>
std::string FormatGenericRange(const T& value)
{
    std::string out{"["};
    size_t count{0};
    for (const auto& elem : value) {
        if (count > 0) out += ", ";
        if (count == 8) {
            out += "...";
            break;
        }
        out += FormatValue(elem);
        ++count;
    }
    out += "]";
    return out;
}

template <typename T>
std::string FormatValueUncaught(const T& value)
{
    if constexpr (IsBool<T>) {
        return value ? "true" : "false";
    } else if constexpr (std::same_as<Decay<T>, std::nullptr_t>) {
        return "nullptr";
    } else if constexpr (IsCharArray<T>) {
        return FormatCharArray(value);
    } else if constexpr (IsCharPointer<T>) {
        return value == nullptr ? "nullptr" : EscapeString(value);
    } else if constexpr (StringValue<T>) {
        return EscapeString(std::string_view{value});
    } else if constexpr (IsOptional<Decay<T>>) {
        return value ? "std::optional(" + FormatValue(*value) + ")"
                     : "std::nullopt";
    } else if constexpr (IsByteValue<T>) {
        return FormatByte(ByteValue(value));
    } else if constexpr (std::is_pointer_v<Decay<T>>) {
        if (value == nullptr) return "nullptr";
        if constexpr (std::is_object_v<std::remove_pointer_t<Decay<T>>>) {
            std::ostringstream oss;
            oss << static_cast<const void*>(value);
            return oss.str();
        } else {
            return "<function pointer>";
        }
    } else if constexpr (std::is_integral_v<Decay<T>>) {
        return std::to_string(value);
    } else if constexpr (std::is_enum_v<Decay<T>>) {
        return FormatValue(static_cast<std::underlying_type_t<Decay<T>>>(value));
    } else if constexpr (CharRange<T>) {
        return FormatCharRange(value);
    } else if constexpr (ByteRange<T>) {
        return FormatByteRange(value);
    } else if constexpr (GenericRange<T>) {
        return FormatGenericRange(value);
    } else if constexpr (Streamable<T>) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    } else {
        return "<unprintable value>";
    }
}

template <typename T>
std::string FormatValue(const T& value)
{
    try {
        return FormatValueUncaught(value);
    } catch (...) {
        return "<unprintable value>";
    }
}

[[noreturn]] inline void Fail(const std::source_location& loc, std::string_view msg)
{
    if (g_detail_test_only_CheckFailuresAreExceptionsNotAborts) {
        throw NonFatalCheckError{msg, loc};
    }

    try {
        const std::string out{"Test check failed: " + std::string{msg} + "\n"};
        std::fwrite(out.data(), 1, out.size(), stderr);
    } catch (...) {
        constexpr std::string_view fallback{"Test check failed while formatting failure output\n"};
        std::fwrite(fallback.data(), 1, fallback.size(), stderr);
    }
    std::abort();
}

template <typename Fn>
[[noreturn]] void FailWith(Fn&& fn, const std::source_location& loc)
{
    try {
        Fail(loc, std::invoke(std::forward<Fn>(fn)));
    } catch (const NonFatalCheckError&) {
        throw;
    } catch (...) {
        Fail(loc, "test check failed while formatting diagnostic");
    }
}

template <typename T>
void CheckBool(const T& value, std::string_view expr, const std::source_location& loc)
{
    bool passed;
    try {
        passed = static_cast<bool>(value);
    } catch (...) {
        FailWith([&] {
            return "expression threw while converting to bool\n  expression: " + std::string{expr};
        }, loc);
    }

    if (!passed) {
        FailWith([&] {
            return "expression evaluated to false\n  expression: " + std::string{expr} +
                   "\n  value: " + FormatValue(value);
        }, loc);
    }
}

template <typename T, typename Fn>
void CheckBoolMessage(const T& value,
                      std::string_view expr,
                      Fn&& fn,
                      const std::source_location& loc)
{
    bool passed;
    try {
        passed = static_cast<bool>(value);
    } catch (...) {
        FailWith([&] {
            return "expression threw while converting to bool\n  expression: " + std::string{expr} +
                   "\n  message: " + StreamMessage(std::forward<Fn>(fn));
        }, loc);
    }

    if (!passed) {
        FailWith([&] {
            return "expression evaluated to false\n  expression: " + std::string{expr} +
                   "\n  value: " + FormatValue(value) +
                   "\n  message: " + StreamMessage(std::forward<Fn>(fn));
        }, loc);
    }
}

enum class ComparisonOp {
    EQ,
    NE,
    LT,
    LE,
    GT,
    GE,
};

constexpr std::string_view OpString(ComparisonOp op)
{
    switch (op) {
    case ComparisonOp::EQ:
        return "==";
    case ComparisonOp::NE:
        return "!=";
    case ComparisonOp::LT:
        return "<";
    case ComparisonOp::LE:
        return "<=";
    case ComparisonOp::GT:
        return ">";
    case ComparisonOp::GE:
        return ">=";
    }
    return "?";
}

template <ComparisonOp OP, typename L, typename R>
bool ComparisonHolds(const L& lhs, const R& rhs)
{
    if constexpr (OP == ComparisonOp::EQ) {
        return lhs == rhs;
    } else if constexpr (OP == ComparisonOp::NE) {
        return lhs != rhs;
    } else if constexpr (OP == ComparisonOp::LT) {
        return lhs < rhs;
    } else if constexpr (OP == ComparisonOp::LE) {
        return lhs <= rhs;
    } else if constexpr (OP == ComparisonOp::GT) {
        return lhs > rhs;
    } else {
        return lhs >= rhs;
    }
}

template <ComparisonOp OP, typename L, typename R>
void CheckComparison(const L& lhs,
                     const R& rhs,
                     std::string_view lhs_expr,
                     std::string_view rhs_expr,
                     const std::source_location& loc)
{
    static_assert(ValidComparisonTypes<L, R>,
                  "CHECK_* comparison operands must not be raw pointers or arrays, bool must be compared with bool, and integral operands must have matching signedness");

    bool passed;
    try {
        passed = ComparisonHolds<OP>(lhs, rhs);
    } catch (...) {
        FailWith([&] {
            return "comparison threw\n  expression: " + std::string{lhs_expr} + " " +
                   std::string{OpString(OP)} + " " + std::string{rhs_expr};
        }, loc);
    }

    if (!passed) {
        FailWith([&] {
            return "comparison failed\n  expression: " + std::string{lhs_expr} + " " +
                   std::string{OpString(OP)} + " " + std::string{rhs_expr} +
                   "\n  lhs: " + FormatValue(lhs) +
                   "\n  rhs: " + FormatValue(rhs);
        }, loc);
    }
}

template <typename I1, typename S1, typename I2, typename S2>
void CheckEqualCollections(I1 first1,
                           S1 last1,
                           I2 first2,
                           S2 last2,
                           std::string_view first1_expr,
                           std::string_view last1_expr,
                           std::string_view first2_expr,
                           std::string_view last2_expr,
                           const std::source_location& loc)
{
    using L = std::iter_reference_t<I1>;
    using R = std::iter_reference_t<I2>;
    static_assert(ValidComparisonTypes<L, R>,
                  "CHECK_EQUAL_COLLECTIONS element types must not be raw pointers or arrays, bool must be compared with bool, and integral operands must have matching signedness");

    size_t index{0};
    for (;;) {
        const bool end1{first1 == last1};
        const bool end2{first2 == last2};
        if (end1 && end2) return;
        if (end1 || end2) {
            FailWith([&] {
                return "range comparison failed\n  expression: [" + std::string{first1_expr} + ", " +
                       std::string{last1_expr} + ") == [" + std::string{first2_expr} + ", " +
                       std::string{last2_expr} + ")\n  mismatch index: " + std::to_string(index) +
                       "\n  lhs: " + (end1 ? std::string{"<end>"} : FormatValue(*first1)) +
                       "\n  rhs: " + (end2 ? std::string{"<end>"} : FormatValue(*first2));
            }, loc);
        }

        bool equal;
        try {
            equal = *first1 == *first2;
        } catch (...) {
            FailWith([&] {
                return "range element comparison threw\n  expression: [" + std::string{first1_expr} +
                       ", " + std::string{last1_expr} + ") == [" + std::string{first2_expr} +
                       ", " + std::string{last2_expr} + ")\n  mismatch index: " +
                       std::to_string(index);
            }, loc);
        }
        if (!equal) {
            FailWith([&] {
                return "range comparison failed\n  expression: [" + std::string{first1_expr} + ", " +
                       std::string{last1_expr} + ") == [" + std::string{first2_expr} + ", " +
                       std::string{last2_expr} + ")\n  mismatch index: " + std::to_string(index) +
                       "\n  lhs: " + FormatValue(*first1) +
                       "\n  rhs: " + FormatValue(*first2);
            }, loc);
        }
        ++first1;
        ++first2;
        ++index;
    }
}

template <typename Exception>
std::string FormatCaughtException(const Exception& e)
{
    if constexpr (std::derived_from<Decay<Exception>, std::exception>) {
        return "\n  actual exception: " + std::string{typeid(e).name()} +
               "\n  what(): " + FormatValue(std::string_view{e.what()});
    } else {
        return "\n  actual exception: " + std::string{typeid(e).name()} +
               "\n  value: " + FormatValue(e);
    }
}

template <typename Exception, typename Fn, typename Predicate>
void CheckException(Fn&& fn,
                    std::string_view expr,
                    std::string_view exception_expr,
                    Predicate&& predicate,
                    std::string_view predicate_expr,
                    const std::source_location& loc)
{
    static_assert(!std::is_reference_v<Exception>, "CHECK_EXCEPTION exception type must not be a reference");

    try {
        std::invoke(std::forward<Fn>(fn));
    } catch (const Exception& e) {
        bool predicate_passed;
        try {
            predicate_passed = std::invoke(std::forward<Predicate>(predicate), e);
        } catch (...) {
            FailWith([&] {
                return "exception predicate threw\n  expression: " + std::string{expr} +
                       "\n  expected exception: " + std::string{exception_expr} +
                       "\n  predicate: " + std::string{predicate_expr} + FormatCaughtException(e);
            }, loc);
        }
        if (!predicate_passed) {
            FailWith([&] {
                return "exception predicate returned false\n  expression: " + std::string{expr} +
                       "\n  expected exception: " + std::string{exception_expr} +
                       "\n  predicate: " + std::string{predicate_expr} + FormatCaughtException(e);
            }, loc);
        }
        return;
    } catch (const std::exception& e) {
        FailWith([&] {
            return "wrong exception type thrown\n  expression: " + std::string{expr} +
                   "\n  expected exception: " + std::string{exception_expr} +
                   "\n  actual exception: " + std::string{typeid(e).name()} +
                   "\n  what(): " + FormatValue(std::string_view{e.what()});
        }, loc);
    } catch (...) {
        FailWith([&] {
            return "wrong exception type thrown\n  expression: " + std::string{expr} +
                   "\n  expected exception: " + std::string{exception_expr} +
                   "\n  actual exception: non-standard exception";
        }, loc);
    }

    FailWith([&] {
        return "expected exception was not thrown\n  expression: " + std::string{expr} +
               "\n  expected exception: " + std::string{exception_expr};
    }, loc);
}

template <typename Fn>
void CheckNoThrow(Fn&& fn,
                  std::string_view expr,
                  const std::source_location& loc)
{
    try {
        std::invoke(std::forward<Fn>(fn));
    } catch (const std::exception& e) {
        FailWith([&] {
            return "unexpected exception thrown\n  expression: " + std::string{expr} +
                   "\n  actual exception: " + std::string{typeid(e).name()} +
                   "\n  what(): " + FormatValue(std::string_view{e.what()});
        }, loc);
    } catch (...) {
        FailWith([&] {
            return "unexpected exception thrown\n  expression: " + std::string{expr} +
                   "\n  actual exception: non-standard exception";
        }, loc);
    }
}

template <typename L, typename R, typename Tol>
void CheckClose(const L& lhs,
                const R& rhs,
                const Tol& tolerance,
                std::string_view lhs_expr,
                std::string_view rhs_expr,
                std::string_view tolerance_expr,
                const std::source_location& loc)
{
    static_assert(std::is_arithmetic_v<Decay<L>> && std::is_arithmetic_v<Decay<R>> && std::is_arithmetic_v<Decay<Tol>>,
                  "CHECK_CLOSE operands and tolerance must be arithmetic");

    const long double tol{static_cast<long double>(tolerance)};
    if (tol < 0) {
        FailWith([&] {
            return "negative tolerance\n  tolerance: " + FormatValue(tolerance);
        }, loc);
    }

    const long double lhs_ld{static_cast<long double>(lhs)};
    const long double rhs_ld{static_cast<long double>(rhs)};

    if (lhs_ld == rhs_ld) return;

    const long double diff{std::fabs(lhs_ld - rhs_ld)};
    const long double scale{std::max(std::fabs(lhs_ld), std::fabs(rhs_ld))};
    const long double allowed{scale == 0 ? 0 : scale * tol / 100.0L};

    if (diff > allowed) {
        FailWith([&] {
            std::ostringstream oss;
            oss << "values are not within tolerance\n  expression: " << lhs_expr << " ~= " << rhs_expr
                << "\n  lhs: " << FormatValue(lhs)
                << "\n  rhs: " << FormatValue(rhs)
                << "\n  tolerance (%): " << FormatValue(tolerance)
                << "\n  tolerance expression: " << tolerance_expr
                << "\n  diff: " << diff
                << "\n  allowed diff: " << allowed;
            return oss.str();
        }, loc);
    }
}

} // namespace test::check_detail

#define CHECK(condition) \
    ::test::check_detail::CheckBool((condition), #condition, std::source_location::current())

#define CHECK_MESSAGE(condition, message) \
    ::test::check_detail::CheckBoolMessage((condition), #condition, [&](std::ostream& _check_message_oss) { _check_message_oss << message; }, std::source_location::current())

#define CHECK_EQUAL(lhs, rhs) \
    ::test::check_detail::CheckComparison<::test::check_detail::ComparisonOp::EQ>((lhs), (rhs), #lhs, #rhs, std::source_location::current())

#define CHECK_NE(lhs, rhs) \
    ::test::check_detail::CheckComparison<::test::check_detail::ComparisonOp::NE>((lhs), (rhs), #lhs, #rhs, std::source_location::current())

#define CHECK_LT(lhs, rhs) \
    ::test::check_detail::CheckComparison<::test::check_detail::ComparisonOp::LT>((lhs), (rhs), #lhs, #rhs, std::source_location::current())

#define CHECK_LE(lhs, rhs) \
    ::test::check_detail::CheckComparison<::test::check_detail::ComparisonOp::LE>((lhs), (rhs), #lhs, #rhs, std::source_location::current())

#define CHECK_GT(lhs, rhs) \
    ::test::check_detail::CheckComparison<::test::check_detail::ComparisonOp::GT>((lhs), (rhs), #lhs, #rhs, std::source_location::current())

#define CHECK_GE(lhs, rhs) \
    ::test::check_detail::CheckComparison<::test::check_detail::ComparisonOp::GE>((lhs), (rhs), #lhs, #rhs, std::source_location::current())

#define CHECK_EQUAL_COLLECTIONS(first1, last1, first2, last2) \
    ::test::check_detail::CheckEqualCollections((first1), (last1), (first2), (last2), #first1, #last1, #first2, #last2, std::source_location::current())

#define CHECK_EXCEPTION(expr, exception_type, predicate)                        \
    ::test::check_detail::CheckException<exception_type>([&] { (void)(expr); }, \
                                                        #expr,                  \
                                                        #exception_type,        \
                                                        (predicate),            \
                                                        #predicate,             \
                                                        std::source_location::current())

#define CHECK_THROW(expr, exception_type) \
    CHECK_EXCEPTION(expr, exception_type, [](const exception_type&) { return true; })

#define CHECK_NO_THROW(expr) \
    ::test::check_detail::CheckNoThrow([&] { (void)(expr); }, #expr, std::source_location::current())

#define CHECK_CLOSE(lhs, rhs, tolerance) \
    ::test::check_detail::CheckClose((lhs), (rhs), (tolerance), #lhs, #rhs, #tolerance, std::source_location::current())

#define CHECK_FAIL(message) \
    ::test::check_detail::FailWith([&] { return std::string{"failure requested\n  message: "} + ::test::check_detail::StreamMessage([&](std::ostream& _check_fail_oss) { _check_fail_oss << message; }); }, std::source_location::current())

#define CHECK_ERROR(message) CHECK_FAIL(message)

#endif // BITCOIN_TEST_UTIL_CHECK_H
