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

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cfloat>
#include <iostream>

namespace moonlight {
namespace test {
//-------------------------------------------------------------------
class TestException : public core::Exception {
public:
   using core::Exception::Exception;
};

//-------------------------------------------------------------------
class AssertionFailed : public TestException {
public:
   using TestException::TestException;
};

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

   TestSuite& test(std::string name, std::function<void()> test_fn) {
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

   int run(std::ostream& out = std::cout) const {
      int tests_failed = 0;

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
               out << "    FAILED (" << typeid(e).name() << "): " << e.what() << std::endl;
#ifdef MOONLIGHT_ENABLE_STACKTRACE
               out << core::format_stacktrace(core::generate_stacktrace()) << std::endl;
#endif
               if (sys::getenv("MOONLIGHT_TEST_RETHROW")) {
                  throw e;
               }
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
      std::cerr << std::endl << "FATAL: Caught signal " << signal
      << " (" << strsignal(signal) << ")"
      << std::endl;

#ifdef MOONLIGHT_ENABLE_STACKTRACE
      std::cerr << core::format_stacktrace(core::generate_stacktrace()) << std::endl;
#endif
      exit(1);
   }

   std::vector<UnitTest> tests;
   std::string name;
};

//-------------------------------------------------------------------
inline void fail(const std::string& message = "Failed.")
{
   throw TestException(message);
}

//-------------------------------------------------------------------
inline void assert_true(bool expr, const std::string& message = "Assertion failed.") {
   if (! expr) {
      throw AssertionFailed(message);
   }
}

//-------------------------------------------------------------------
inline void assert_false(bool expr, const std::string& message = "Negative assertion failed.") {
   if (expr) {
      throw AssertionFailed(message);
   }
}

//-------------------------------------------------------------------
inline void epsilon_assert(double a, double b,
                           double epsilon = DBL_EPSILON,
                           const std::string& message = "Value equivalence assertion failed. (double epsilon)") {

   if (fabs(a - b) > epsilon) {
      throw AssertionFailed(message);
   }
}

//-------------------------------------------------------------------
inline void epsilon_assert(float a, float b,
                           float epsilon = FLT_EPSILON,
                           const std::string& message = "Value equivalence assertion failed. (float epsilon)") {
   if (fabs(a - b) > epsilon) {
      throw AssertionFailed(message);
   }
}

//-------------------------------------------------------------------
template<class T>
inline void assert_equal(const T& a, const T& b,
                         const std::string& message = "Value equivalence assertion failed.") {
   if (a != b) {
      throw AssertionFailed(message);
   }
}

//-------------------------------------------------------------------
template<>
inline void assert_equal<double>(const double& a, const double& b, const std::string& message) {
   try {
      epsilon_assert(a, b);

   } catch (const AssertionFailed& e) {
      throw AssertionFailed(message);
   }
}

//-------------------------------------------------------------------
template<>
inline void assert_equal<float>(const float& a, const float& b, const std::string& message) {
   try {
      epsilon_assert(a, b);

   } catch (const AssertionFailed& e) {
      throw AssertionFailed(message);
   }
}

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
}
}

#endif /* __MOONLIGHT_TEST_H */
