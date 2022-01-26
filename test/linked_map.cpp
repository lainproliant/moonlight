/*
 * linked_map.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday May 17, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/linked_map.h"
#include "moonlight/maps.h"
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight linked_map tests")
    .die_on_signal(SIGSEGV)
    .test("Insertion order", []() {
        moonlight::linked_map<std::string, int> map;

        map.insert({"oranges", 1});
        map.insert({"grapes", 3});
        map.insert({"bananas", 2});

        for (auto key : maps::keys(map)) {
            std::cout << key << ": " << map[key] << std::endl;
        }

        ASSERT_EQUAL(map["oranges"], 1);
        ASSERT_EQUAL(map["grapes"], 3);
        ASSERT_EQUAL(map["bananas"], 2);

        ASSERT_EQUAL(
            moonlight::maps::keys(map).collect(),
            {"oranges", "grapes", "bananas"});
    })
    .test("Insertion order after removal", []() {
        moonlight::linked_map<std::string, int> map;

        map.insert({"oranges", 1});
        map.insert({"grapes", 3});
        map.insert({"bananas", 2});
        map.insert({"pears", 6});
        map.insert({"apricots", 8});

        map.erase("bananas");

        ASSERT_EQUAL(
            moonlight::maps::keys(map).collect(),
            {"oranges", "grapes", "pears", "apricots"});

    })
    .run();
}
