/*
 * lex.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday February 19, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/lex.h"
#include "moonlight/test.h"
#include "moonlight/file.h"

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

lex::Grammar make_scheme_grammar() {
    lex::Grammar root;
    auto sexpr = root.sub();

    sexpr.ignore("\\s+");
    sexpr.match("`", "quote");
    sexpr.match("[0-9]*.?[0-9]+", "number");
    sexpr.match("[-!?+*/A-Za-z_][-!?+*/A-za-z_0-9]*", "word");
    sexpr.push("\\(", sexpr, "open-paren");
    sexpr.pop("\\)", "close-paren");

    root.ignore("\\s+");
    root.push("\\(", sexpr, "open-paren");
    return root;
}

int main() {
    return TestSuite("moonlight lex tests")
        .die_on_signal(SIGSEGV)
        .test("a simple scheme lexer", []() {
            lex::Lexer lex;
            lex::Grammar g = make_scheme_grammar();
            auto tokens = lex.lex(g, file::slurp("./data/test_scheme"));
            for (auto tk : tokens) {
                std::cout << tk << std::endl;
            }
        })
        .run();
}
