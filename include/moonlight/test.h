/*
 * moonlight/test.h: A very simple to use unit testing framework
 *    built around lambda expressions.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Thu October 9, 2014
 */
#ifndef __MOONLIGHT_TEST_H
#define __MOONLIGHT_TEST_H

#include "moonlight/exceptions.h"
#include "moonlight/system.h"
#include "moonlight/traits.h"

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <iostream>
#include <algorithm>
#include <random>

namespace moonlight {
namespace test {
//-------------------------------------------------------------------
class UnitTest {
public:
    UnitTest(const std::string& name, std::function<void()> test_fn) :
    test_fn(test_fn), name(name) {}
    virtual ~UnitTest() {}

    void run() const {
        test_fn();
    }

    std::string get_name() const {
        return name;
    }

private:
    std::function<void()> test_fn;
    std::string name;
};

//-------------------------------------------------------------------
class TestSuite {
public:
    TestSuite(const std::string& name) : name(name) {}
    virtual ~TestSuite() {}

    TestSuite& test(const std::string& name, std::function<void()> test_fn) {
        return test(UnitTest(name, test_fn));
    }

    TestSuite& test(const UnitTest& test) {
        tests.push_back(test);
        return *this;
    }

    TestSuite& die_on_signal(int signalId) {
        signal(signalId, signal_callback);
        return *this;
    }

    int run(std::ostream& out = std::cout) {
        int tests_failed = 0;

        std::random_device random_device;
        std::mt19937 rng(random_device());

        std::shuffle(tests.begin(), tests.end(), rng);

        out << "===== " << name << " =====" << std::endl;

        for (const UnitTest& test : tests) {
            try {
                out << "Running test: '" << test.get_name() << "'..." << std::endl;
                test.run();
                out << "    PASSED" << std::endl;

            } catch (...) {
                std::exception_ptr eptr = std::current_exception();

                try {
                    std::rethrow_exception(eptr);

                } catch (const std::exception& e) {
                    out << "    FAILED " << e.what() << std::endl;
                    if (sys::getenv("MOONLIGHT_TEST_RETHROW")) {
                        throw e;
                    }

                } catch (...) {
                    out << "    FAILED (exotic type thrown)" <<  std::endl;
#ifdef MOONLIGHT_ENABLE_STACKTRACE
                    auto trace = debug::StackTrace::generate();
                    trace.format(out, "        at ");
#endif
                }

                tests_failed ++;
            }
        }

        out << std::endl;

        return tests_failed;
    }

    int size() const {
        return tests.size();
    }

private:
    static void signal_callback(int signal) {
        std::cout << std::endl << "FATAL: Caught signal " << signal
        << " (" << strsignal(signal) << ")"
        << std::endl;

#ifdef MOONLIGHT_ENABLE_STACKTRACE
        std::cout << debug::StackTrace::generate({}, 3) << std::endl;
#endif
        exit(1);
    }

    std::vector<UnitTest> tests;
    std::string name;
};

//-------------------------------------------------------------------
#define _ASSERT(expr, msg, repr) \
    if (! (expr)) { \
        THROW(moonlight::core::AssertionFailure, msg ": " repr); \
    }

#define ASSERT_TRUE(expr) _ASSERT(expr, "Assertion failed", #expr)
#define ASSERT ASSERT_TRUE
#define ASSERT_FALSE(expr) _ASSERT(!(expr), "Negative assertion failed.", #expr)

//-------------------------------------------------------------------
template<class T>
inline size_t generic_list_size(const T& listA) {
    size_t sz = 0;
    for (auto iter = listA.begin(); iter != listA.end(); iter++) {
        sz++;
    }
    return sz;
}

//-------------------------------------------------------------------
template<class T>
inline bool lists_equal(const T& listA, const T& listB) {

    return generic_list_size(listA) == generic_list_size(listB) &&
    equal(listA.begin(), listA.end(), listB.begin());
}

//-------------------------------------------------------------------
template<class M>
inline bool maps_equal(const M& mapA, const M& mapB) {
    if (mapA.size() != mapB.size()) {
        return false;
    }

    for (auto iterA = mapA.begin(); iterA != mapA.end(); iterA++) {
        auto iterB = mapB.find(iterA->first);

        if (iterB == mapB.end() || iterA->second != iterB->second) {
            return false;
        }
    }

    return true;
}

//-------------------------------------------------------------------
template<class T>
inline bool test_equal(const T& a, const T& b) {
    if constexpr(is_map_type<T>()) {
        return maps_equal(a, b);
    }

    if constexpr (is_iterable_type<T>()) {
        return lists_equal(a, b);
    }


    return a == b;
}

//-------------------------------------------------------------------
inline bool test_equal(const std::string& a, const char* b) {
    return a == b;
}

//-------------------------------------------------------------------
inline bool test_equal(const char* a, const std::string& b) {
    return b == a;
}

//-------------------------------------------------------------------
template<class T>
inline bool ep_test_equal(T a, T b, T ep) {
    return fabs(a - b) <= ep;
}

//-------------------------------------------------------------------
template<double>
inline bool test_equal(double a, double b) {
    return ep_test_equal(a, b, DBL_EPSILON);
}

//-------------------------------------------------------------------
template<float>
inline bool test_equal(float a, float b) {
    return ep_test_equal(a, b, FLT_EPSILON);
}

#define ASSERT_EQUAL(...) _ASSERT(test_equal(__VA_ARGS__), "Value equivalence assertion failed", #__VA_ARGS__)
#define ASSERT_NOT_EQUAL(...) _ASSERT(!test_equal(__VA_ARGS__), "Value inequivalence assertion failed", #__VA_ARGS__)
#define ASSERT_EP_EQUAL(...) _ASSERT(ep_test_equal(__VA_ARGS__), "Value equivalence assertion failed", #__VA_ARGS__)

}
}

#endif /* __MOONLIGHT_TEST_H */
