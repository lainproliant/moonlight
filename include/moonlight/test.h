/*
 * ## test.h: A unit testing framework built around closures. -------
 *
 * Author: Lain Supe (lainproliant)
 * Date: Thu October 9, 2014
 *
 * ## Usage ---------------------------------------------------------
 * `moonlight::test` is a simple to use unit testing framework built around
 * lambda expressions, i.e. closures.  The quickest way to get started writing
 * your unit tests is to build a `test::TestSuite` object using its builder
 * function `test()`, then call `run()` to run the tests.  See the example below:
 *
 * ```
 * int main() {
 *      return TestSuite("My Unit Tests")
 *      .test("add two numbers", []() {
 *          ASSERT_EQUAL(2, 1 + 1);
 *      })
 *      .test("concatenate two strings", []() {
 *          std::string strA = "hello ";
 *          std::string strB = "world!";
 *          std::string result = "hello, world!";
 *          ASSERT_EQUAL(result, strA + strB);
 *      })
 *      .run();
 * }
 * ```
 *
 * The following methods are defined on `TestSuite`:
 * - `test(name, f())`: Defines a test in the suite with the given name.  When
 *   tests are run, they are run in a random order regardless of the order in
 *   which they were defined.
 * - `run()`: Runs the unit tests in the suite in random order, returning the
 *   number of failed tests.
 *
 * The following assertion macros are defined for use in tests, which throw
 * `core::AssertionFailure` if the assertions are false.
 *
 * - `ASSERT_EQUAL(a, b)`: Asserts that the two objects `a` and `b` are equal.
 *   This macro allows comparison between a wide variety of objects and iterable
 *   collections.
 * - `ASSERT_NOT_EQUAL(a, b)` Asserts that `a` and `b` are not equal.
 * - `ASSERT_EP_EQUAL(a, b, e)`: Asserts that `a` and `b` are equal within the
 *   given epislon variance `e`.  Use this to compare doubles or floats if the
 *   default `DBL_EPSILON` and `FLT_EPSILON` used by `ASSERT_EQUAL` are not
 *   suitable for the comparison.
 * - `FAIL(msg)`: Use this macro to trigger an assertion failure in your test
 *   with the given explanation message.
 *
 * There are a few different ways in which you can compose your different test
 * suites.  For smaller sets of tests, it may be appropriate to just define all
 * of your tests in a single test suite.  For larger code bases, like
 * `moonlight` itself, I have chosen to define each test suite in context of
 * each header in the `./test` source directory each in their own source files.
 * Then, `build.py` manages compiling and running the test suites, and contains
 * logic to randomize the order in which the test suites are run.
 */
#ifndef __MOONLIGHT_TEST_H
#define __MOONLIGHT_TEST_H

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <iostream>
#include <algorithm>
#include <random>
#include <vector>
#include <string>

#include "moonlight/exceptions.h"
#include "moonlight/system.h"
#include "moonlight/traits.h"

namespace moonlight {
namespace test {
//-------------------------------------------------------------------
class UnitTest {
 public:
     UnitTest(const std::string& name, std::function<void()> test_fn) :
     test_fn(test_fn), name(name) {}
     virtual ~UnitTest() {}

     void run(std::ostream& out = std::cout) const {
         int cycles = 1;
         std::optional<std::string> cycles_str = sys::getenv("MOONLIGHT_TEST_CYCLES");
         if (cycles_str.has_value()) {
             cycles = std::stoi(cycles_str.value());
         }

         for (int x = 0; x < cycles; x++) {
             test_fn();
             if (cycles > 1) {
                 out << "   [ " << x+1 << " / " << cycles << " ]" << std::endl;
             }
         }
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
     explicit TestSuite(const std::string& name) : name(name) {}
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
             if (sys::getenv("MOONLIGHT_TEST_UNGUARDED")) {
                 test.run();
                 out << "    PASSED" << std::endl;
                 continue;
             }

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

}  // namespace test
}  // namespace moonlight

#endif /* __MOONLIGHT_TEST_H */
