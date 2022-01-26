/*
 * slice.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include <vector>
#include "moonlight/slice.h"
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::test;

static const std::vector<int> array = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

int main() {
    return TestSuite("moonlight slice tests")
    .test("slice simple tests", []() {
        ASSERT_EQUAL({0, 1, 2}, slice(array, {}, 3));
        ASSERT_EQUAL({7, 8, 9}, slice(array, -3, {}));
        ASSERT_EQUAL({5, 6, 7}, slice(array, -5, -2));
    })
    .test("no out of bounds error in range", []() {
        ASSERT_EQUAL({0, 1}, slice(array, -500, 2));
    })
    .test("out of bounds error in offset", []() {
        try {
            slice(array, -100);
            FAIL("Expected IndexError was not thrown.");

        } catch (const core::IndexError& e) {
            std::cerr << "Caught expected " << e << std::endl;
        }
    })
    .run();
}
