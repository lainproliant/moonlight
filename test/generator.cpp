/*
 * generator.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include <thread>
#include <chrono>
#include <optional>
#include <functional>

#include "moonlight/json.h"
#include "moonlight/generator.h"
#include "moonlight/test.h"

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

struct Person {
    std::string name;
    std::string address;

    json::Object to_json() const {
        return json::Object()
        .set("name", name)
        .set("address", address);
    }
};

const std::vector<Person> PEOPLE = {
    {.name="Lain", .address="Silverdale, WA"},
    {.name="Jenna", .address="Bremerton, WA"},
    {.name="Boz", .address="Pet Lodge"},
};

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
    .test("stream left trim", []() {
        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        auto new_values = gen::stream(values).trim_left(2).collect();
        std::cout << "values = " << str::join(values, ",") << std::endl;
        std::cout << "new_values = " << str::join(new_values, ",") << std::endl;
        ASSERT_EQUAL(new_values, {2, 3, 4, 5});

    })
    .test("stream right trim", []() {
        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        auto new_values = gen::stream(values).trim_right(3).collect();
        std::cout << "values = " << str::join(values, ",") << std::endl;
        std::cout << "new_values = " << str::join(new_values, ",") << std::endl;
        ASSERT_EQUAL(new_values, {0, 1, 2});
    })
    .test("stream trim", []() {
        std::vector<int> values = {0, 1, 2, 3, 4, 5};
        auto new_values = gen::stream(values).trim(1, 2).collect();
        std::cout << "values = " << str::join(values, ",") << std::endl;
        std::cout << "new_values = " << str::join(new_values, ",") << std::endl;
        ASSERT_EQUAL(new_values, {1, 2, 3});
    })
    .test("map collect", []() {
        std::map<std::string, int> map({
            {"Apples", 1},
            {"Oranges", 2},
            {"BANANAS", 3}
        });

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

        auto vec = gen::stream({1, 2, 3, 4}).collect();
        ASSERT_EQUAL(vec.size(), (size_t)4);
        std::cout << str::join(vec, ", ") << std::endl;
        ASSERT_EQUAL(vec, {1, 2, 3, 4});

        gen::Stream<int> sA = gen::stream(vA);
        gen::Stream<int> sB = gen::stream(vB);
        gen::Stream<int> sC = gen::stream(vC);

        auto result = gen::stream({sA, sB, sC}).sum().collect();

        std::cout << str::join(result, ", ") << std::endl;

        ASSERT_EQUAL(result, {1, 2, 3, 4, 5, 6, 7, 8, 9});

        auto stream_coll = std::vector<gen::Stream<int>>{
            gen::stream(vA),
            gen::stream(vB),
            gen::stream(vC)
        };

        auto coll_result = gen::stream(stream_coll).sum().collect();
        ASSERT_EQUAL(coll_result, {1, 2, 3, 4, 5, 6, 7, 8, 9});

    })
    .test("stream join str and filter", []() {
        auto stream = gen::stream(range(0, 1000000))
        .filter([](auto& i) {
            return i % 65537 == 0;
        });
        auto s = stream.join(",");
        std::cout << s << std::endl;
        ASSERT_EQUAL(s, "0,65537,131074,196611,262148,327685,393222,458759,524296,589833,655370,720907,786444,851981,917518,983055");
    })
    .test("Concatenate two Stream objects", []() {
        auto streamA = gen::stream(range(0, 10));
        auto streamB = gen::stream(range(10, 20));
        auto streamC = gen::stream(range(0, 20));


        auto streamAB = streamA + streamB;
        auto sAB = streamAB.join(",");
        auto sC = streamC.join(",");
        std::cout << sAB << std::endl;
        std::cout << sC << std::endl;

        ASSERT_EQUAL(sAB, sC);
    })
    .test("Concatenate in place one Stream object onto another", []() {
        auto streamA = gen::stream(range(0, 10));
        auto streamB = gen::stream(range(10, 20));
        auto streamC = gen::stream(range(0, 20));

        streamA += streamB;
        auto sA = streamA.join(",");
        auto sC = streamC.join(",");
        std::cout << sA << std::endl;
        std::cout << sC << std::endl;
        ASSERT_EQUAL(sA, sC);
    })
    .test("Transform an array into JSON.", []() {
        auto obj = json::Object()
        .set("addresses", gen::stream(PEOPLE).transform<json::Object>([](auto& v) { return v.to_json(); }));

        auto address_list = obj.get<json::Array>("addresses");
        ASSERT_EQUAL(address_list.get<json::Object>(0).get<std::string>("name"), "Lain");
        ASSERT_EQUAL(address_list.get<json::Object>(1).get<std::string>("name"), "Jenna");
        ASSERT_EQUAL(address_list.get<json::Object>(2).get<std::string>("name"), "Boz");

        std::cout << json::to_string(obj) << std::endl;
        std::cout << json::to_string(address_list) << std::endl;
    })
    .run();
}
