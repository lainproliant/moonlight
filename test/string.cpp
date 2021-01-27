#include "moonlight/core.h"
#include "moonlight/test.h"

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight string.h tests")
    .die_on_signal(SIGSEGV)
    .test("str::startswith", []() {
        assert_true(str::startswith("oranges", "ora"));
        assert_false(str::startswith("oranges", "oraz"));
    })
    .test("str::endswith", []() {
        assert_true(str::endswith("oranges", "ges"));
        assert_false(str::endswith("oranges", "gesz"));
    })
    .test("str::join", []() {
        vector<int> v = {1, 2, 3, 4};
        assert_equal(str::join(v, ","), string("1,2,3,4"));
    })
    .test("str::split", []() {
        vector<string> tokens;
        str::split(tokens, "1,2,3,4", ",");
        assert_true(lists_equal(tokens, {"1", "2", "3", "4"}));
        assert_true(lists_equal(str::split("1:2:3:4", ":"), {"1", "2", "3", "4"}));
    })
    .test("str::chr", []() {
        assert_equal(str::chr('c'), string("c"));
    })
    .test("str::trim, str::trim_left, str::trim_right", []() {
        string s = "   abc   ";
        cout << "trim_left: \"" << str::trim_left(s) << "\"" << endl;
        cout << "trim_right: \"" << str::trim_right(s) << "\"" << endl;
        cout << "trim: \"" << str::trim(s) << "\"" << endl;
        assert_equal(str::trim_left(s), string("abc   "));
        assert_equal(str::trim_right(s), string("   abc"));
        assert_equal(str::trim(s), string("abc"));
    })
    .test("str::literal", []() {
        string s = "\xa9 oranges \n";
        string repr = "\\xa9 oranges \\n";
        cout << "s = " << s << std::endl;
        cout << "repr = " << repr << std::endl;
        cout << "str::literal(s) = " << str::literal(s) << std::endl;
        assert_equal(repr, str::literal(s));
    })
    .run();
}
