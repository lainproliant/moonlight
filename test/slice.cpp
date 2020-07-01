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
        assert_true(lists_equal({0, 1, 2}, slice(array, {}, 3)));
        assert_true(lists_equal({7, 8, 9}, slice(array, -3, {})));
        assert_true(lists_equal({5, 6, 7}, slice(array, -5, -2)));
    })
    .test("no out of bounds error in range", []() {
        assert_true(lists_equal({0, 1}, slice(array, -500, 2)));
    })
    .test("out of bounds error in offset", []() {
        try {
            slice(array, -100);
            return false;

        } catch (const SliceError& e) {
            std::cerr << "Received expected SliceError: "
                      << e.get_message()
                      << std::endl;
            return true;
        }
    })
    .run();
}
