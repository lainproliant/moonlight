#include "moonlight/test.h"

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight string.h tests")
    .die_on_signal(SIGSEGV)
    .test("str::startswith", []() {
        ASSERT(str::startswith("oranges", "ora"));
        ASSERT_FALSE(str::startswith("oranges", "oraz"));
    })
    .test("str::endswith", []() {
        ASSERT(str::endswith("oranges", "ges"));
        ASSERT_FALSE(str::endswith("oranges", "gesz"));
    })
    .test("str::join", []() {
        vector<int> v = {1, 2, 3, 4};
        ASSERT_EQUAL(str::join(v, ","), string("1,2,3,4"));
    })
    .test("str::split", []() {
        vector<string> tokens;
        str::split(tokens, "1,2,3,4", ",");
        ASSERT_EQUAL(tokens, {"1", "2", "3", "4"});
        ASSERT_EQUAL(str::split("1:2:3:4", ":"), {"1", "2", "3", "4"});
    })
    .test("str::chr", []() {
        ASSERT_EQUAL(str::chr('c'), string("c"));
    })
    .test("str::trim, str::trim_left, str::trim_right", []() {
        string s = "   abc   ";
        cout << "trim_left: \"" << str::trim_left(s) << "\"" << endl;
        cout << "trim_right: \"" << str::trim_right(s) << "\"" << endl;
        cout << "trim: \"" << str::trim(s) << "\"" << endl;
        ASSERT_EQUAL(str::trim_left(s), string("abc   "));
        ASSERT_EQUAL(str::trim_right(s), string("   abc"));
        ASSERT_EQUAL(str::trim(s), string("abc"));
    })
    .test("str::literal", []() {
        string s = "\xa9 oranges \n";
        string repr = "\\xa9 oranges \\n";
        cout << "s = " << s << std::endl;
        cout << "repr = " << repr << std::endl;
        cout << "str::literal(s) = " << str::literal(s) << std::endl;
        ASSERT_EQUAL(repr, str::literal(s));
    })
    .run();
}
