/*
 * shlex.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday June 7, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/shlex.h"
#include "moonlight/test.h"

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight shlex tests")
    .test("shlex split - simple", []() {
        auto result = shlex::split("a b c d");
        assert_equal(result.size(), (size_t)4);

        auto joined = shlex::join(result);
        assert_equal(joined, std::string("a b c d"));
    })
    .test("shlex split - complex", []() {
        std::string cmd = "\'banana cream \"\" \\\'pie\\\'\' oranges \"pineapple \n\n\"";
        auto result = shlex::split(cmd);
        assert_equal(result.size(), (size_t)3);

        auto joined = shlex::join(result);
        assert_equal(joined, std::string("\'banana cream \"\" \'\"\'\"\'pie\'\"\'\"\'\' oranges \'pineapple \n\n\'"));

        auto split = shlex::split(joined);
        assert_equal(split.size(), (size_t)3);

        auto joined_again = shlex::join(result);
        assert_equal(joined_again, std::string("\'banana cream \"\" \'\"\'\"\'pie\'\"\'\"\'\' oranges \'pineapple \n\n\'"));

        auto split_again = shlex::split(joined_again);
        assert_equal(split.size(), (size_t)3);
    })
    .run();

}
