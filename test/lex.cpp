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

using Grammar = lex::Grammar<std::string>;

Grammar make_scheme_grammar() {
    auto root = Grammar();
    auto sexpr = root.sub();
    auto ignore_whitespace = lex::ignore("\\s");

    sexpr
    .def(ignore_whitespace)
    .def(lex::match("`"), "quote")
    .def(lex::match("[0-9]*.?[0-9]+"), "number")
    .def(lex::match("[-!?+*/A-Za-z_!][-!?+*/A-za-z_0-9!]*"), "word")
    .def(lex::match("[\\+-/=]"), "operator")
    .def(lex::push("\\(", sexpr), "open-paren")
    .def(lex::pop("\\)"), "close-paren");

    root
    .def(ignore_whitespace)
    .def(lex::push("\\(", sexpr), "open-paren");

    return root;
}

enum Abba {
    A,
    B
};

using AbbaGrammar = lex::Grammar<Abba>;

AbbaGrammar make_enum_abba_grammar() {
    return AbbaGrammar()
    .def(lex::match("a"), Abba::A)
    .def(lex::pop("b"), Abba::B);
}

Grammar make_abba_grammar() {
    return Grammar()
    .def(lex::match("a"), "a")
    .def(lex::pop("b"), "b");
}

int main() {
    return TestSuite("moonlight lex tests")
    .die_on_signal(SIGSEGV)
    .test("a simple scheme lexer", []() {
        auto g = make_scheme_grammar();
        auto lex = g.lexer();

        auto tokens = lex.lex(file::slurp("test/data/test_scheme"));
        for (auto tk : tokens) {
            std::cout << tk << std::endl;
        }
    })
    .test("popping from root state ends parsing", []() {
        auto abba = make_abba_grammar();
        auto lex = abba.lexer().throw_on_error(false);

        auto tokens = lex.lex("aaabaaa");
        for (auto token : tokens) {
            std::cout << token << std::endl;
        }
        ASSERT_EQUAL(tokens.size(), 4ul);
    })
    .test("unexpected characters", []() {
        auto abba = make_abba_grammar();
        auto lex = abba.lexer();

        try {
            auto tokens = lex.lex("acab");
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
        auto abba = make_abba_grammar();
        auto abra = Grammar()
        .inherit(abba)
        .def(lex::match("abra"), "abra");

        auto lex = abra.lexer();
        auto tokens = lex.lex("aaaabraabraab");
        for (auto tk : tokens) {
            std::cout << tk << std::endl;
        }

        ASSERT_EQUAL(tokens.size(), 7ul);
    })
    .test("grammar with enum-named states", []() {
        auto abba = make_enum_abba_grammar();
        auto lex = abba.lexer().throw_on_error(false);
        auto tokens = lex.lex("abaaa");
        ASSERT_EQUAL(tokens.size(), 2ul);
        ASSERT_EQUAL(tokens[0].type(), Abba::A);
        ASSERT_EQUAL(tokens[1].type(), Abba::B);
    })
    .run();
}
