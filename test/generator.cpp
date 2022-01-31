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

        ASSERT_EQUAL(results, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    })
    .test("confirm async generator works properly", []() {
        auto queue = gen::async(sleepy_range(0, 10));
        std::vector<int> results;

        auto future = queue->process_async([&](int x) {
            results.push_back(x);
        });
        future.get();
        ASSERT_EQUAL(results, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
    })
    .test("wrapping other iterator pairs into a generator.", []() {
        std::vector<int> values = {1, 2, 3, 4};
        std::vector<int> copy;

        for (auto iter = gen::wrap<std::vector<int>::iterator>(values.begin(), values.end());
             iter != gen::end<int>();
             iter++) {
            copy.push_back(*iter);
        }
        ASSERT_EQUAL(values, copy);
    })
    .test("buffered stream", []() {
        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        auto stream = gen::buffer(gen::stream(values), 3);

        auto value_arrays = stream.transform<std::vector<int>>([](gen::Buffer<int> buf) -> std::optional<std::vector<int>> {
            std::vector<int> result;
            std::copy(buf->begin(), buf->end(), std::back_inserter(result));
            return result;
        }).collect();

        ASSERT_EQUAL(value_arrays, {
            {0, 1, 2},
            {1, 2, 3},
            {2, 3, 4},
            {3, 4, 5}
        });
    })
    .test("squash buffered stream", []() {
        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        auto stream = gen::buffer(gen::stream(values), 3, true);
        auto new_values = stream.transform<int>([](gen::Buffer<int> buf) -> std::optional<int> {
            return buf->front();
        }).collect();

        std::cout << "values = " << str::join(values, ",") << std::endl;
        std::cout << "new_values = " << str::join(new_values, ",") << std::endl;
        ASSERT_EQUAL(values, new_values);
    })
    .run();
}
