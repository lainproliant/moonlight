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
        ASSERT_EQUAL(result.size(), (size_t)4);

        auto joined = shlex::join(result);
        ASSERT_EQUAL(joined, std::string("a b c d"));
    })
    .test("shlex split - complex", []() {
        std::string cmd = "\'banana cream \"\" \\\'pie\\\'\' oranges \"pineapple \n\n\"";
        auto result = shlex::split(cmd);
        ASSERT_EQUAL(result.size(), (size_t)3);

        auto joined = shlex::join(result);
        ASSERT_EQUAL(joined, std::string("\'banana cream \"\" \'\"\'\"\'pie\'\"\'\"\'\' oranges \'pineapple \n\n\'"));

        auto split = shlex::split(joined);
        ASSERT_EQUAL(split.size(), (size_t)3);

        auto joined_again = shlex::join(result);
        ASSERT_EQUAL(joined_again, std::string(
                "\'banana cream \"\" \'\"\'\"\'pie\'\"\'\"\'\' oranges \'pineapple \n\n\'"));

        auto split_again = shlex::split(joined_again);
        ASSERT_EQUAL(split.size(), (size_t)3);
    })
    .test("shlex comments", []() {
        std::string lineA = "#this is a comment";
        std::string lineB = "# this is a comment";
        std::string lineC = "this is#a comment";
        std::string lineD = "this is #a comment";
        std::string lineE = "this is # a comment";

        ASSERT_EQUAL(shlex::split(lineA), {});
        ASSERT_EQUAL(shlex::split(lineB), {});
        ASSERT_EQUAL(shlex::split(lineC), {"this", "is"});
        ASSERT_EQUAL(shlex::split(lineD), {"this", "is"});
        ASSERT_EQUAL(shlex::split(lineE), {"this", "is"});
    })
    .run();

}
