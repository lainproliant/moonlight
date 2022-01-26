#include <iostream>
#include "moonlight/test.h"

using namespace std;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight TestSuite (test.h)")
    .test("Forced failure by assertion", []() {
        ostream cnull(0);
        ASSERT_EQUAL(TestSuite("internal test suite")
                     .test("doomed test", []() {
                         ASSERT_TRUE(false);
                     })
                     .run(cnull), 1);
    })
    .test("Forced failure by runtime exception", []() {
        ostream cnull(0);
        ASSERT_EQUAL(TestSuite("internal test suite")
                     .test("doomed test", []() {
                         throw runtime_error("oh noes!");
                     })
                     .run(cnull), 1);
    })
    .run();
}
