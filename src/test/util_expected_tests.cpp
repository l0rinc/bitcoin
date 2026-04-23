// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <test/util/setup_common.h>
#include <util/expected.h>

#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>
#include <utility>


using namespace util;

BOOST_AUTO_TEST_SUITE(util_expected_tests)

BOOST_AUTO_TEST_CASE(expected_value)
{
    struct Obj {
        int x;
    };
    Expected<Obj, int> e{};
    CHECK_EQUAL(e.value().x, std::remove_cvref_t<decltype(e.value().x)>{0});

    e = Obj{42};

    CHECK(e.has_value());
    CHECK(static_cast<bool>(e));
    CHECK_EQUAL(e.value().x, std::remove_cvref_t<decltype(e.value().x)>{42});
    CHECK_EQUAL((*e).x, std::remove_cvref_t<decltype((*e).x)>{42});
    CHECK_EQUAL(e->x, std::remove_cvref_t<decltype(e->x)>{42});

    // modify value
    e.value().x += 1;
    (*e).x += 1;
    e->x += 1;

    const auto& read{e};
    CHECK_EQUAL(read.value().x, std::remove_cvref_t<decltype(read.value().x)>{45});
    CHECK_EQUAL((*read).x, std::remove_cvref_t<decltype((*read).x)>{45});
    CHECK_EQUAL(read->x, std::remove_cvref_t<decltype(read->x)>{45});
}

BOOST_AUTO_TEST_CASE(expected_value_rvalue)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(5)};
    const auto moved{std::move(no_copy).value()};
    CHECK_EQUAL(*moved, std::remove_cvref_t<decltype(*moved)>{5});
}

BOOST_AUTO_TEST_CASE(expected_deref_rvalue)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(5)};
    const auto moved{*std::move(no_copy)};
    CHECK_EQUAL(*moved, std::remove_cvref_t<decltype(*moved)>{5});
}

BOOST_AUTO_TEST_CASE(expected_value_or)
{
    Expected<std::unique_ptr<int>, int> no_copy{std::make_unique<int>(1)};
    const int one{*std::move(no_copy).value_or(std::make_unique<int>(2))};
    CHECK_EQUAL(one, std::remove_cvref_t<decltype(one)>{1});

    const Expected<std::string, int> const_val{Unexpected{-1}};
    CHECK_EQUAL(const_val.value_or("fallback"), std::string_view{"fallback"});
}

BOOST_AUTO_TEST_CASE(expected_value_throws)
{
    const Expected<int, std::string> e{Unexpected{"fail"}};
    CHECK_THROW(e.value(), BadExpectedAccess);

    const Expected<void, std::string> void_e{Unexpected{"fail"}};
    CHECK_THROW(void_e.value(), BadExpectedAccess);
}

BOOST_AUTO_TEST_CASE(expected_error)
{
    Expected<void, std::string> e{};
    CHECK(e.has_value());
    [&]() -> void { return e.value(); }(); // check value returns void and does not throw
    [&]() -> void { return *e; }();

    e = Unexpected{"fail"};
    CHECK(!e.has_value());
    CHECK(!static_cast<bool>(e));
    CHECK_EQUAL(e.error(), std::string_view{"fail"});

    // modify error
    e.error() += "1";

    const auto& read{e};
    CHECK_EQUAL(read.error(), std::string_view{"fail1"});
}

BOOST_AUTO_TEST_CASE(expected_error_rvalue)
{
    {
        Expected<int, std::unique_ptr<int>> nocopy_err{Unexpected{std::make_unique<int>(7)}};
        const auto moved{std::move(nocopy_err).error()};
        CHECK_EQUAL(*moved, std::remove_cvref_t<decltype(*moved)>{7});
    }
    {
        Expected<void, std::unique_ptr<int>> void_nocopy_err{Unexpected{std::make_unique<int>(9)}};
        const auto moved{std::move(void_nocopy_err).error()};
        CHECK_EQUAL(*moved, std::remove_cvref_t<decltype(*moved)>{9});
    }
}

BOOST_AUTO_TEST_CASE(unexpected_error_accessors)
{
    Unexpected u{std::make_unique<int>(-1)};
    CHECK_EQUAL(*u.error(), -1);

    *u.error() -= 1;
    const auto& read{u};
    CHECK_EQUAL(*read.error(), -2);

    const auto moved{std::move(u).error()};
    CHECK_EQUAL(*moved, -2);
}

BOOST_AUTO_TEST_CASE(expected_swap)
{
    Expected<const char*, std::unique_ptr<int>> a{Unexpected{std::make_unique<int>(-1)}};
    Expected<const char*, std::unique_ptr<int>> b{"good"};
    a.swap(b);
    CHECK_EQUAL(a.value(), std::string_view{"good"});
    CHECK_EQUAL(*b.error(), -1);
}

BOOST_AUTO_TEST_SUITE_END()
