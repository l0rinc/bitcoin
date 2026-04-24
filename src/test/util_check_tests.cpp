// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>

#include <test/util/check.h>

#include <boost/test/unit_test.hpp>
#include <test/util/common.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

BOOST_AUTO_TEST_SUITE(util_check_tests)

static_assert(test::check_detail::ValidComparisonTypes<int, int>);
static_assert(test::check_detail::ValidComparisonTypes<unsigned, uint64_t>);
static_assert(test::check_detail::ValidComparisonTypes<std::string, std::string_view>);
static_assert(!test::check_detail::ValidComparisonTypes<int, unsigned>);
static_assert(!test::check_detail::ValidComparisonTypes<bool, int>);
static_assert(!test::check_detail::ValidComparisonTypes<int*, int*>);
static_assert(test::check_detail::ValidComparisonTypes<decltype("literal"), std::string>);
static_assert(test::check_detail::ValidComparisonTypes<const char*, std::string>);

template <typename Fn>
std::string CheckFailureMessage(Fn&& fn)
{
    test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};
    try {
        fn();
    } catch (const NonFatalCheckError& e) {
        return e.what();
    }
    throw std::logic_error{"check did not fail"};
}

struct ThrowingPrintable {
    int value;
    friend bool operator==(const ThrowingPrintable& a, const ThrowingPrintable& b) { return a.value == b.value; }
    friend std::ostream& operator<<(std::ostream&, const ThrowingPrintable&) { throw std::runtime_error{"format failed"}; }
};

struct NonStandardException {
};

struct WithToString {
    std::string ToString() const { return "with-to-string"; }
    friend bool operator==(const WithToString&, const WithToString&) = default;
};

BOOST_AUTO_TEST_CASE(check_pass)
{
    Assume(true);
    Assert(true);
    CHECK_NONFATAL(true);
}

BOOST_AUTO_TEST_CASE(check_fail)
{
    // Disable aborts for easier testing here
    test_only_CheckFailuresAreExceptionsNotAborts mock_checks{};

    if constexpr (G_ABORT_ON_FAILED_ASSUME) {
        CHECK_EXCEPTION(Assume(false), NonFatalCheckError, HasReason{"Internal bug detected: false"});
    } else {
        CHECK_NO_THROW(Assume(false));
    }
    CHECK_EXCEPTION(Assert(false), NonFatalCheckError, HasReason{"Internal bug detected: false"});
    CHECK_EXCEPTION(CHECK_NONFATAL(false), NonFatalCheckError, HasReason{"Internal bug detected: false"});
}

BOOST_AUTO_TEST_CASE(test_check_pass)
{
    CHECK(true);
    CHECK_EQUAL(std::remove_cvref_t<decltype(std::remove_cvref_t<decltype(1)>{1})>{1}, std::remove_cvref_t<decltype(1)>{1});
    CHECK_NE(1, 2);
    CHECK_LT(1, 2);
    CHECK_LE(1, 1);
    CHECK_GT(2, 1);
    CHECK_GE(2, 2);

    const std::array<std::byte, 2> bytes{std::byte{0x01}, std::byte{0x02}};
    CHECK_EQUAL_COLLECTIONS(bytes.begin(), bytes.end(), bytes.begin(), bytes.end());
    CHECK_EXCEPTION(throw std::runtime_error{"expected reason"}, std::runtime_error, HasReason{"expected reason"});
    CHECK_THROW(throw std::runtime_error{"expected reason"}, std::runtime_error);
    CHECK_NO_THROW((void)0);
    CHECK_MESSAGE(true, "message ok");
    CHECK_CLOSE(99.5, 100.0, 1.0);
    CHECK_EQUAL(std::optional<int>{1}, std::optional<int>{1});
    CHECK_EQUAL(WithToString{}, WithToString{});

    std::thread thread{[] { CHECK(true); }};
    thread.join();
}

BOOST_AUTO_TEST_CASE(test_check_fail)
{
    auto message{CheckFailureMessage([] { CHECK(false); })};
    CHECK_NE(message.find("expression evaluated to false"), std::string::npos);
    CHECK_NE(message.find("expression: false"), std::string::npos);

    int* null_ptr{nullptr};
    message = CheckFailureMessage([&] { CHECK(null_ptr); });
    CHECK_NE(message.find("value: nullptr"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_EQUAL(std::remove_cvref_t<decltype(std::remove_cvref_t<decltype(1)>{2})>{1}, std::remove_cvref_t<decltype(1)>{2}); });
    CHECK_NE(message.find("expression: 1 == 2"), std::string::npos);
    CHECK_NE(message.find("lhs: 1"), std::string::npos);
    CHECK_NE(message.find("rhs: 2"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_EQUAL(std::byte{0x1b}, std::byte{0x02}); });
    CHECK_NE(message.find("lhs: 0x1b"), std::string::npos);
    CHECK_NE(message.find("rhs: 0x02"), std::string::npos);

    const std::string left{"a\n"};
    const std::string right{"a\t"};
    message = CheckFailureMessage([&] { CHECK_EQUAL(left, right); });
    CHECK_NE(message.find("lhs: \"a\\n\""), std::string::npos);
    CHECK_NE(message.find("rhs: \"a\\t\""), std::string::npos);

    const std::vector<unsigned char> mid_left{1, 2, 3};
    const std::vector<unsigned char> mid_right{1, 9, 3};
    message = CheckFailureMessage([&] {
        CHECK_EQUAL_COLLECTIONS(mid_left.begin(), mid_left.end(), mid_right.begin(), mid_right.end());
    });
    CHECK_NE(message.find("mismatch index: 1"), std::string::npos);
    CHECK_NE(message.find("lhs: 0x02"), std::string::npos);
    CHECK_NE(message.find("rhs: 0x09"), std::string::npos);

    const std::vector<unsigned char> end_left{1};
    const std::vector<unsigned char> end_right{1, 2};
    message = CheckFailureMessage([&] {
        CHECK_EQUAL_COLLECTIONS(end_left.begin(), end_left.end(), end_right.begin(), end_right.end());
    });
    CHECK_NE(message.find("mismatch index: 1"), std::string::npos);
    CHECK_NE(message.find("lhs: <end>"), std::string::npos);
    CHECK_NE(message.find("rhs: 0x02"), std::string::npos);

    message = CheckFailureMessage([] {
        CHECK_EXCEPTION((void)0, std::runtime_error, [](const std::runtime_error&) { return true; });
    });
    CHECK_NE(message.find("expected exception was not thrown"), std::string::npos);

    message = CheckFailureMessage([] {
        CHECK_EXCEPTION(throw std::logic_error{"wrong\nreason"}, std::runtime_error, [](const std::runtime_error&) { return true; });
    });
    CHECK_NE(message.find("wrong exception type thrown"), std::string::npos);
    CHECK_NE(message.find("what(): \"wrong\\nreason\""), std::string::npos);

    message = CheckFailureMessage([] {
        CHECK_EXCEPTION(throw NonStandardException{}, std::runtime_error, [](const std::runtime_error&) { return true; });
    });
    CHECK_NE(message.find("actual exception: non-standard exception"), std::string::npos);

    message = CheckFailureMessage([] {
        CHECK_EXCEPTION(throw std::runtime_error{"predicate reason"}, std::runtime_error, [](const std::runtime_error&) { return false; });
    });
    CHECK_NE(message.find("exception predicate returned false"), std::string::npos);
    CHECK_NE(message.find("what(): \"predicate reason\""), std::string::npos);

    message = CheckFailureMessage([] {
        CHECK_EXCEPTION(throw NonStandardException{}, NonStandardException, [](const NonStandardException&) { return false; });
    });
    CHECK_NE(message.find("exception predicate returned false"), std::string::npos);
    CHECK_NE(message.find("value: <unprintable value>"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_EQUAL(ThrowingPrintable{1}, ThrowingPrintable{2}); });
    CHECK_NE(message.find("lhs: <unprintable value>"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_MESSAGE(false, "details " << 7); });
    CHECK_NE(message.find("message: details 7"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_NO_THROW(throw std::runtime_error{"boom"}); });
    CHECK_NE(message.find("unexpected exception thrown"), std::string::npos);
    CHECK_NE(message.find("what(): \"boom\""), std::string::npos);

    message = CheckFailureMessage([] { CHECK_THROW((void)0, std::runtime_error); });
    CHECK_NE(message.find("expected exception was not thrown"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_CLOSE(100.0, 103.0, 1.0); });
    CHECK_NE(message.find("values are not within tolerance"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_FAIL("forced " << 9); });
    CHECK_NE(message.find("failure requested"), std::string::npos);
    CHECK_NE(message.find("message: forced 9"), std::string::npos);

    message = CheckFailureMessage([] { CHECK_ERROR("forced " << 10); });
    CHECK_NE(message.find("message: forced 10"), std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
