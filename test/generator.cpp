/*
 * generator.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/generator.h"
#include "moonlight/test.h"

#include <thread>
#include <chrono>
#include <optional>
#include <functional>

using namespace moonlight;
using namespace moonlight::test;

std::function<std::optional<int>()> range(int a, int b) {
    int y = a;

    auto lambda = [=]() mutable -> std::optional<int> {
        if (y >= b) {
            return {};
        } else {
            return y++;
        }
    };

    return lambda;
}

std::function<std::optional<int>()> sleepy_range(int a, int b) {
    int y = a;

    auto lambda = [=]() mutable -> std::optional<int> {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        if (y >= b) {
            return {};
        } else {
            return y++;
        }
    };

    return lambda;
}

int main() {
    return TestSuite("moonlight generator tests")
    .test("confirm generator works properly", []() {
        std::vector<int> results;

        for (auto iter = gen::begin(range(0, 10));
             iter != gen::end<int>();
             iter++) {
            results.push_back(*iter);
        }

        assert_true(lists_equal(results, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    })
    .test("confirm async generator works properly", []() {
        auto queue = gen::async(sleepy_range(0, 10));
        std::vector<int> results;

        auto future = queue->process_async([&](int x) {
            results.push_back(x);
        });
        future.get();
        assert_true(lists_equal(results, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
    })
    .test("wrapping other iterator pairs into a generator.", []() {
        std::vector<int> values = {1, 2, 3, 4};
        std::vector<int> copy;

        for (auto iter = gen::wrap<std::vector<int>::iterator>(values.begin(), values.end());
             iter != gen::end<int>();
             iter++) {
            copy.push_back(*iter);
        }
        assert_true(lists_equal(values, copy));
    })
    .run();
}
