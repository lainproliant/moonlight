/*
 * generator.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/generator.h"
#include "moonlight/linked_map.h"
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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

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
        return true;
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
        auto stream = gen::stream(values).buffer(3);

        auto value_arrays = stream.transform<std::vector<int>>([](gen::Buffer<int>& buf) {
            std::vector<int> result;
            std::copy(buf.begin(), buf.end(), std::back_inserter(result));
            std::cout << "result = " << str::join(result, ",") << std::endl;
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
        auto stream = gen::stream(values).buffer(3, true);
        auto new_values = stream.transform<int>([](gen::Buffer<int>& buf) {
            return buf.front();
        }).collect();

        std::cout << "values = " << str::join(values, ",") << std::endl;
        std::cout << "new_values = " << str::join(new_values, ",") << std::endl;
        ASSERT_EQUAL(values, new_values);
    })
    .test("stream trim", []() {
        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        auto new_values = gen::stream(values).trim(1, 2).collect();
        std::cout << "values = " << str::join(values, ",") << std::endl;
        std::cout << "new_values = " << str::join(new_values, ",") << std::endl;
        ASSERT_EQUAL(new_values, {1, 2, 3});
    })
    .test("map collect", []() {
        std::map<std::string, int> map = {
            {"Apples", 1},
            {"Oranges", 2},
            {"BANANAS", 3}
        };

        auto remap = gen::stream(map).map_collect<std::string, int>([](const auto& pair) {
            return std::pair(str::to_lower(pair.first), pair.second);
        });

        for (auto pair : remap) {
            std::cout << pair.first << " = " << pair.second << std::endl;
        }

        ASSERT_EQUAL(remap, std::unordered_map<std::string, int>({
            {"apples", 1},
            {"oranges", 2},
            {"bananas", 3}
        }));
    })
    .test("reduce", []() {
        std::vector<int> values = {10, 11, 9, 10};
        auto stream = gen::stream(values);

        auto enumerate = stream.reduce<std::pair<int, int>>([](const auto& acc, const auto& value) {
            return std::pair(acc.first + 1, acc.second + value);
        });

        auto sum = stream.sum();
        auto avg = enumerate.second / enumerate.first;

        ASSERT_EQUAL(avg, 10);
        ASSERT_EQUAL(sum, 40);
    })
    .test("stream merge", []() {
        std::vector<int> vA = {1, 2, 3};
        std::vector<int> vB = {4, 5, 6};
        std::vector<int> vC = {7, 8, 9};

        auto vec = gen::seq(1, 2, 3, 4).collect();
        ASSERT_EQUAL(vec.size(), (size_t)4);
        std::cout << str::join(vec, ", ") << std::endl;
        ASSERT_EQUAL(vec, {1, 2, 3, 4});

        auto result = gen::merge(gen::seq(
            gen::stream(vA),
            gen::stream(vB),
            gen::stream(vC)
        )).collect();

        ASSERT_EQUAL(result, {1, 2, 3, 4, 5, 6, 7, 8, 9});

        auto stream_coll = std::vector<gen::Stream<int>>{
            gen::stream(vA),
            gen::stream(vB),
            gen::stream(vC)
        };

        auto coll_result = gen::merge(gen::stream(stream_coll)).collect();
        //ASSERT_EQUAL(coll_result, {1, 2, 3, 4, 5, 6, 7, 8, 9});

    })
    .run();
}
