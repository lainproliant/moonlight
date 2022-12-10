/*
 * rx.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday December 9, 2022
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/rx.h"
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight rx tests")
    .die_on_signal(SIGSEGV)
    .test("Floating point regex", []() {
        const auto RX = rx::def("^[-+]?([0-9]+(\\.[0-9]+)?|\\.[0-9]+)$");
        ASSERT_TRUE(rx::match(RX, "+1234"));
        ASSERT_TRUE(rx::match(RX, "-123.4"));
        ASSERT_TRUE(rx::match(RX, "-.4"));
        ASSERT_FALSE(rx::match(RX, "12.3.4"));
        ASSERT_FALSE(rx::match(RX, "34."));
    })
    .test("Regex component capture", []() {
        const auto RX = rx::def("^([0-9]+)\\.([0-9]+)$");
        const rx::Capture cp = rx::capture(RX, "123.456");
        ASSERT_TRUE(cp);
        ASSERT_EQUAL(cp.groups().size(), 3ul);
        ASSERT_EQUAL(cp.group(), "123.456");
        ASSERT_EQUAL(cp.group(1), "123");
        ASSERT_EQUAL(cp.group(2), "456");
    })
    .run();
}
