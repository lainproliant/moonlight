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

lex::Grammar::Pointer make_scheme_grammar() {
    auto root = lex::Grammar::create();
    auto sexpr = root->sub();
    auto ignore_whitespace = lex::ignore("\\s");

    sexpr
        ->def(ignore_whitespace)
        ->def(lex::match("`"), "quote")
        ->def(lex::match("[0-9]*.?[0-9]+"), "number")
        ->def(lex::match("[-!?+*/A-Za-z_!][-!?+*/A-za-z_0-9!]*"), "word")
        ->def(lex::match("[\\+-/=]"), "operator")
        ->def(lex::push("\\(", sexpr), "open-paren")
        ->def(lex::pop("\\)"), "close-paren");

    root
        ->def(ignore_whitespace)
        ->def(lex::push("\\(", sexpr), "open-paren");

    return root;
}

lex::Grammar::Pointer make_abba_grammar() {
    auto root = lex::Grammar::create();
    root
        ->def(lex::match("a"), "a")
        ->def(lex::pop("b"), "b");
    return root;
}

int main() {
    return TestSuite("moonlight lex tests")
        .die_on_signal(SIGSEGV)
        .test("a simple scheme lexer", []() {
            lex::Lexer lex;
            auto g = make_scheme_grammar();
            auto tokens = lex.lex(g, file::slurp("test/data/test_scheme"));
            for (auto tk : tokens) {
                std::cout << tk << std::endl;
            }
        })
        .test("popping from root state ends parsing", []() {
            lex::Lexer lex = lex::Lexer().throw_on_error(false);
            auto abba = make_abba_grammar();

            auto tokens = lex.lex(abba, "aaabaaa");
            ASSERT_EQUAL(tokens.size(), 4ul);
        })
        .test("unexpected characters", []() {
            lex::Lexer lex;
            auto abba = make_abba_grammar();

            try {
                auto tokens = lex.lex(abba, "acab");
                for (auto tk : tokens) {
                    std::cout << tk << std::endl;
                }

            } catch (const core::ValueError& e) {
                std::cout << "Caught expected ValueError." << std::endl;
            } catch (...) {
                FAIL("An unexpected error was thrown.");
            }
        })
        .test("inheriting grammars", []() {
            lex::Lexer lex;
            auto abba = make_abba_grammar();

            auto abra = lex::Grammar::create();
            abra
                ->inherit(abba)
                ->def(lex::match("abra"), "abra");

            auto tokens = lex.lex(abra, "aaaabraabraab");
            for (auto tk : tokens) {
                std::cout << tk << std::endl;
            }

            ASSERT_EQUAL(tokens.size(), 7ul);
        })
        .run();
}
