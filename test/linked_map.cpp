/*
 * linked_map.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday May 17, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/linked_map.h"
#include "moonlight/test.h"


using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight linked_map tests")
    .die_on_signal(SIGSEGV)
    .test("Insertion order", []()->bool {
        moonlight::linked_map<std::string, int> map;

        map.insert({"oranges", 1});
        map.insert({"grapes", 3});
        map.insert({"bananas", 2});

        for (auto key : moonlight::maps::keys(map)) {
            std::cout << "Key: " << key << std::endl;
        }

        assert_equal(map["oranges"], 1);
        assert_equal(map["grapes"], 3);
        assert_equal(map["bananas"], 2);

        assert_true(
            lists_equal(
                moonlight::maps::keys(map),
                {"oranges", "grapes", "bananas"}));
        return true;
    })
    .test("Insertion order after removal", []()->bool {
        moonlight::linked_map<std::string, int> map;

        map.insert({"oranges", 1});
        map.insert({"grapes", 3});
        map.insert({"bananas", 2});
        map.insert({"pears", 6});
        map.insert({"apricots", 8});

        map.erase("bananas");

        assert_true(
            lists_equal(
                moonlight::maps::keys(map),
                {"oranges", "grapes", "pears", "apricots"}));

        return true;
    })
    .run();
}
