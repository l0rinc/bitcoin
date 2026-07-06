// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <prevector.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>

#include <algorithm>
#include <boost/test/unit_test.hpp>

#include <ranges>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(prevector_tests, BasicTestingSetup)

template <unsigned int N, typename T>
class prevector_tester
{
    typedef std::vector<T> realtype;
    realtype real_vector;
    realtype real_vector_alt;

    typedef prevector<N, T> pretype;
    pretype pre_vector;
    pretype pre_vector_alt;

    typedef typename pretype::size_type Size;
    bool passed = true;
    uint256 rand_seed;


    template <typename A, typename B>
    void local_check_equal(A a, B b)
    {
        local_check(a == b);
    }
    void local_check(bool b)
    {
        passed &= b;
    }
    void test()
    {
        const pretype& const_pre_vector = pre_vector;
        local_check_equal(real_vector.size(), pre_vector.size());
        local_check_equal(real_vector.empty(), pre_vector.empty());
        for (Size s = 0; s < real_vector.size(); s++) {
            local_check(real_vector[s] == pre_vector[s]);
            local_check(&(pre_vector[s]) == &(pre_vector.begin()[s]));
            local_check(&(pre_vector[s]) == &*(pre_vector.begin() + s));
            local_check(&(pre_vector[s]) == &*((pre_vector.end() + s) - real_vector.size()));
        }
        // local_check(realtype(pre_vector) == real_vector);
        local_check(pretype(real_vector.begin(), real_vector.end()) == pre_vector);
        local_check(pretype(pre_vector.begin(), pre_vector.end()) == pre_vector);
        size_t pos = 0;
        for (const T& v : pre_vector) {
            local_check(v == real_vector[pos++]);
        }
        for (const T& v : pre_vector | std::views::reverse) {
            local_check(v == real_vector[--pos]);
        }
        for (const T& v : const_pre_vector) {
            local_check(v == real_vector[pos++]);
        }
        for (const T& v : const_pre_vector | std::views::reverse) {
            local_check(v == real_vector[--pos]);
        }
        DataStream ss1{};
        DataStream ss2{};
        ss1 << real_vector;
        ss2 << pre_vector;
        local_check_equal(ss1.size(), ss2.size());
        for (Size s = 0; s < ss1.size(); s++) {
            local_check_equal(ss1[s], ss2[s]);
        }
        const bool real_less{std::lexicographical_compare(real_vector.begin(), real_vector.end(),
                                                          real_vector_alt.begin(), real_vector_alt.end())};
        const bool real_alt_less{std::lexicographical_compare(real_vector_alt.begin(), real_vector_alt.end(),
                                                              real_vector.begin(), real_vector.end())};
        local_check_equal(pre_vector < pre_vector_alt, real_less);
        local_check_equal(pre_vector_alt < pre_vector, real_alt_less);
        local_check(!(pre_vector < pre_vector));
        local_check(!(pre_vector_alt < pre_vector_alt));
    }

public:
    void resize(Size s)
    {
        real_vector.resize(s);
        local_check_equal(real_vector.size(), s);
        pre_vector.resize(s);
        local_check_equal(pre_vector.size(), s);
        test();
    }

    void reserve(Size s)
    {
        real_vector.reserve(s);
        local_check(real_vector.capacity() >= s);
        pre_vector.reserve(s);
        local_check(pre_vector.capacity() >= s);
        test();
    }

    void insert(Size position, const T& value)
    {
        real_vector.insert(real_vector.begin() + position, value);
        pre_vector.insert(pre_vector.begin() + position, value);
        test();
    }

    void insert(Size position, Size count, const T& value)
    {
        real_vector.insert(real_vector.begin() + position, count, value);
        pre_vector.insert(pre_vector.begin() + position, count, value);
        test();
    }

    template <typename I>
    void insert_range(Size position, I first, I last)
    {
        real_vector.insert(real_vector.begin() + position, first, last);
        pre_vector.insert(pre_vector.begin() + position, first, last);
        test();
    }

    void erase(Size position)
    {
        real_vector.erase(real_vector.begin() + position);
        pre_vector.erase(pre_vector.begin() + position);
        test();
    }

    void erase(Size first, Size last)
    {
        real_vector.erase(real_vector.begin() + first, real_vector.begin() + last);
        pre_vector.erase(pre_vector.begin() + first, pre_vector.begin() + last);
        test();
    }

    void update(Size pos, const T& value)
    {
        real_vector[pos] = value;
        pre_vector[pos] = value;
        test();
    }

    void push_back(const T& value)
    {
        real_vector.push_back(value);
        pre_vector.push_back(value);
        test();
    }

    void pop_back()
    {
        real_vector.pop_back();
        pre_vector.pop_back();
        test();
    }

    void clear()
    {
        real_vector.clear();
        pre_vector.clear();
    }

    void assign(Size n, const T& value)
    {
        real_vector.assign(n, value);
        pre_vector.assign(n, value);
    }

    Size size() const
    {
        return real_vector.size();
    }

    Size capacity() const
    {
        return pre_vector.capacity();
    }

    void shrink_to_fit()
    {
        pre_vector.shrink_to_fit();
        local_check_equal(pre_vector.capacity(), std::max<size_t>(static_cast<size_t>(N), pre_vector.size()));
        test();
    }

    void swap() noexcept
    {
        real_vector.swap(real_vector_alt);
        pre_vector.swap(pre_vector_alt);
        test();
    }

    void move()
    {
        real_vector = std::move(real_vector_alt);
        real_vector_alt.clear();
        pre_vector = std::move(pre_vector_alt);
        pre_vector_alt.clear();
    }

    void copy()
    {
        real_vector = real_vector_alt;
        pre_vector = pre_vector_alt;
    }

    void resize_uninitialized(realtype values)
    {
        size_t r = values.size();
        size_t s = real_vector.size() / 2;
        if (real_vector.capacity() < s + r) {
            real_vector.reserve(s + r);
        }
        real_vector.resize(s);
        pre_vector.resize_uninitialized(s);
        for (auto v : values) {
            real_vector.push_back(v);
        }
        auto p = pre_vector.size();
        pre_vector.resize_uninitialized(p + r);
        for (auto v : values) {
            pre_vector[p] = v;
            ++p;
        }
        test();
    }

    ~prevector_tester()
    {
        BOOST_CHECK_MESSAGE(passed, "insecure_rand: " + rand_seed.ToString());
    }

    prevector_tester(FastRandomContext& rng)
    {
        rand_seed = rng.rand256();
        rng.Reseed(rand_seed);
    }
};

BOOST_AUTO_TEST_CASE(PrevectorTestInt)
{
    for (int j = 0; j < 64; j++) {
        prevector_tester<8, int> test{m_rng};
        for (int i = 0; i < 2048; i++) {
            if (m_rng.randbits(2) == 0) {
                test.insert(m_rng.randrange(test.size() + 1), int(m_rng.rand32()));
            }
            if (test.size() > 0 && m_rng.randbits(2) == 1) {
                test.erase(m_rng.randrange(test.size()));
            }
            if (m_rng.randbits(3) == 2) {
                int new_size = std::max(0, std::min(30, (int)test.size() + (int)m_rng.randrange(5) - 2));
                test.resize(new_size);
            }
            if (m_rng.randbits(3) == 3) {
                test.insert(m_rng.randrange(test.size() + 1), 1 + m_rng.randbool(), int(m_rng.rand32()));
            }
            if (m_rng.randbits(3) == 4) {
                int del = std::min<int>(test.size(), 1 + (m_rng.randbool()));
                int beg = m_rng.randrange(test.size() + 1 - del);
                test.erase(beg, beg + del);
            }
            if (m_rng.randbits(4) == 5) {
                test.push_back(int(m_rng.rand32()));
            }
            if (test.size() > 0 && m_rng.randbits(4) == 6) {
                test.pop_back();
            }
            if (m_rng.randbits(5) == 7) {
                int values[4];
                int num = 1 + (m_rng.randbits(2));
                for (int k = 0; k < num; k++) {
                    values[k] = int(m_rng.rand32());
                }
                test.insert_range(m_rng.randrange(test.size() + 1), values, values + num);
            }
            if (m_rng.randbits(5) == 8) {
                int del = std::min<int>(test.size(), 1 + (m_rng.randbits(2)));
                int beg = m_rng.randrange(test.size() + 1 - del);
                test.erase(beg, beg + del);
            }
            if (m_rng.randbits(5) == 9) {
                test.reserve(m_rng.randbits(5));
            }
            if (m_rng.randbits(6) == 10) {
                test.shrink_to_fit();
            }
            if (test.size() > 0) {
                test.update(m_rng.randrange(test.size()), int(m_rng.rand32()));
            }
            if (m_rng.randbits(10) == 11) {
                test.clear();
            }
            if (m_rng.randbits(9) == 12) {
                test.assign(m_rng.randbits(5), int(m_rng.rand32()));
            }
            if (m_rng.randbits(3) == 3) {
                test.swap();
            }
            if (m_rng.randbits(4) == 8) {
                test.copy();
            }
            if (m_rng.randbits(5) == 18) {
                test.move();
            }
            if (m_rng.randbits(5) == 19) {
                unsigned int num = 1 + (m_rng.randbits(4));
                std::vector<int> values(num);
                for (int& v : values) {
                    v = int(m_rng.rand32());
                }
                test.resize_uninitialized(values);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(prevector_lexicographical_order)
{
    using PreVec = prevector<8, int>;
    const auto make_prevector{[](const std::vector<int>& values) {
        return PreVec{values.begin(), values.end()};
    }};

    const std::vector<int> high_first_short{2};
    const std::vector<int> low_first_long{1, 3};
    const PreVec pre_high_first_short{make_prevector(high_first_short)};
    const PreVec pre_low_first_long{make_prevector(low_first_long)};

    BOOST_CHECK_EQUAL(pre_low_first_long < pre_high_first_short,
                      std::lexicographical_compare(low_first_long.begin(), low_first_long.end(),
                                                   high_first_short.begin(), high_first_short.end()));
    BOOST_CHECK_EQUAL(pre_high_first_short < pre_low_first_long,
                      std::lexicographical_compare(high_first_short.begin(), high_first_short.end(),
                                                   low_first_long.begin(), low_first_long.end()));

    const std::vector<int> prefix{1};
    const std::vector<int> extension{1, 0};
    const PreVec pre_prefix{make_prevector(prefix)};
    const PreVec pre_extension{make_prevector(extension)};

    BOOST_CHECK_EQUAL(pre_prefix < pre_extension,
                      std::lexicographical_compare(prefix.begin(), prefix.end(), extension.begin(), extension.end()));
    BOOST_CHECK_EQUAL(pre_extension < pre_prefix,
                      std::lexicographical_compare(extension.begin(), extension.end(), prefix.begin(), prefix.end()));
}

BOOST_AUTO_TEST_CASE(prevector_shrink_to_fit_capacity)
{
    using PreVec = prevector<8, int>;

    PreVec direct_sized;
    direct_sized.reserve(32);
    for (int i{0}; i < 4; ++i) {
        direct_sized.push_back(i);
    }
    BOOST_CHECK_GT(direct_sized.capacity(), PreVec::STATIC_SIZE);
    direct_sized.shrink_to_fit();
    BOOST_CHECK_EQUAL(direct_sized.size(), 4);
    BOOST_CHECK_EQUAL(direct_sized.capacity(), PreVec::STATIC_SIZE);
    BOOST_CHECK_EQUAL(direct_sized.allocated_memory(), 0);
    for (int i{0}; i < 4; ++i) {
        BOOST_CHECK_EQUAL(direct_sized[i], i);
    }

    PreVec indirect_sized;
    indirect_sized.reserve(32);
    for (int i{0}; i < 10; ++i) {
        indirect_sized.push_back(i);
    }
    BOOST_CHECK_GT(indirect_sized.capacity(), indirect_sized.size());
    indirect_sized.shrink_to_fit();
    BOOST_CHECK_EQUAL(indirect_sized.size(), 10);
    BOOST_CHECK_EQUAL(indirect_sized.capacity(), indirect_sized.size());
    BOOST_CHECK_EQUAL(indirect_sized.allocated_memory(), indirect_sized.size() * sizeof(int));
    for (int i{0}; i < 10; ++i) {
        BOOST_CHECK_EQUAL(indirect_sized[i], i);
    }
}

BOOST_AUTO_TEST_SUITE_END()
