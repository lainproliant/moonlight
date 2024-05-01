/*
 * functional_map.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday May 1, 2024
 */

#include "moonlight/functional_map.h"
#include "moonlight/test.h"

using namespace moonlight::test;

int main() {
    return TestSuite("functional_mapping tests")
    .die_on_signal(SIGSEGV)
    .test("basic usage test", []() {
        moonlight::FunctionalMap<int> fib;

        fib[0] = 0;
        fib[([](auto v) { return v <= 1; })] = 1;
        fib.otherwise() = [&](auto v) {
            return fib(v-1) + fib(v-2);
        };

        ASSERT_EQUAL(fib(1), 1);
        ASSERT_EQUAL(fib(2), 1);
        ASSERT_EQUAL(fib(3), 2);
        ASSERT_EQUAL(fib(4), 3);
        ASSERT_EQUAL(fib(5), 5);
        ASSERT_EQUAL(fib(6), 8);
        ASSERT_EQUAL(fib(7), 13);
    })
    .test("self-referential mappings", []() {
        moonlight::FunctionalMap<int> fmap;
        fmap[0] = 1;
        fmap[1] = fmap.of(0);
        fmap[2] = [=](auto v) { return v*v; };
        fmap[8] = fmap.of(2);
        fmap.otherwise() = fmap.of(0);

        ASSERT_EQUAL(fmap(0), 1);
        ASSERT_EQUAL(fmap(1), 1);
        ASSERT_EQUAL(fmap(2), 4);
        ASSERT_EQUAL(fmap(8), 64);
        ASSERT_EQUAL(fmap(10), 1);
        ASSERT_EQUAL(fmap(100), 1);
    })
    .test("multi-predicate mappings", []() {
        moonlight::FunctionalMap<int, bool> fmap;
        fmap[99, [](auto& v) { return v > 0 && v % 100 == 1; }] = true;
        fmap[([](auto& v) { return v > 0 && v % 2 == 0; })] = true;
        fmap[0, 1, 2] = false;
        fmap.otherwise() = false;

        ASSERT_EQUAL(fmap(0), false);
        ASSERT_EQUAL(fmap(1), true);
        ASSERT_EQUAL(fmap(2), true);
        ASSERT_EQUAL(fmap(3), false);
        ASSERT_EQUAL(fmap(4), true);
        ASSERT_EQUAL(fmap(99), true);
        ASSERT_EQUAL(fmap(100), true);
        ASSERT_EQUAL(fmap(101), true);
        ASSERT_EQUAL(fmap(103), false);
    })
    .run();
}
