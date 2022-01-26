/*
 * shlex.h
 *
 * An interpretation of a subset of the functionality of the
 * shlex Python module.
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday June 7, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_SHLEX_H
#define __MOONLIGHT_SHLEX_H

#include <istream>
#include <sstream>
#include <string>
#include <memory>
#include <set>
#include <regex>
#include "moonlight/generator.h"
#include "moonlight/collect.h"
#include "moonlight/file.h"

namespace moonlight {
namespace shlex {

//-------------------------------------------------------------------
class ShellLexer {
public:
    static std::string quote(const std::string& str) {
        if (str.size() == 0) {
            return "''";
        }

        if (std::regex_search(str, rx_unsafe())) {
            std::ostringstream sb;
            std::string result;
            std::regex_replace(std::back_inserter(result), str.begin(),
                               str.end(), rx_quotes(), "\'\"$1\"\'");
            sb << '\'' << result << '\'';
            return sb.str();

        } else {
            return str;
        }
    }

    gen::Iterator<std::string> begin() {
        // TODO
        return gen::begin<std::string>(std::bind(&ShellLexer::read_token, this));
    }

    gen::Iterator<std::string> end() {
        return gen::end<std::string>();
    }

    ShellLexer& punctuation(const std::string& punctuation) {
        _punctuation.clear();
        _punctuation.insert(punctuation.begin(), punctuation.end());
        return *this;
    }

protected:
    virtual file::BufferedInput& input() = 0;

private:
    std::optional<std::string> read_token() {
        if (_punctuation.find(input().peek()) != _punctuation.end()) {
            int c = input().getc();
            return str::chr(c);
        }

        switch (input().peek()) {
        case '\n':
            input().advance();
            return "\n";
        case '\'':
            return parse_single_quotes();
        case '\"':
            return parse_double_quotes();
        case '#':
            while (input().peek() != EOF && input().peek() != '\n') input().advance();
            return read_token();
        case EOF:
            return {};
        case ' ':
        case '\t':
            while (isspace(input().peek()) && input().peek() != '\n') input().advance();
            return read_token();
        default:
            return parse_word();
        }
    }

    std::string parse_single_quotes() {
        input().advance();
        std::string str;
        for (;;) {
            int c = input().getc();

            if (c == EOF) {
                THROW(core::ValueError, "Unterminated single-quote string.");
            }

            if (c == '\\') {
                c = input().peek();

                if (c == EOF) {
                    THROW(core::ValueError, "Incomplete escape sequence in single-quote string.");
                }

                if (c == '\\' || c == '\'') {
                    input().advance();
                    str.push_back(c);

                } else {
                    str.push_back('\\');
                }


            } else if (c == '\'') {
                break;

            } else {
                str.push_back(c);
            }
        }

        if (input().peek() == '\"' || input().peek() == '\'') {
            str += read_token().value();
        }

        return str;
    }

    std::string parse_double_quotes() {
        input().advance();
        std::string str;
        for (;;) {
            int c = input().getc();

            if (c == EOF) {
                THROW(core::ValueError, "Unterminated double-quote string.");
            }

            if (c == '\\') {
                c = input().peek();

                if (c == EOF) {
                    THROW(core::ValueError, "Incomplete escape sequence in double-quote string.");
                }

                auto iter = escape_map().find(c);
                if (iter != escape_map().end()) {
                    input().advance();
                    str.push_back(iter->second);

                } else {
                    THROW(core::ValueError, "Unrecognized escape sequence in double-quote string.");
                }
            } else if (c == '\"') {
                break;
            } else {
                str.push_back(c);
            }
        }

        if (input().peek() == '\"' || input().peek() == '\'') {
            str += read_token().value();
        }

        return str;
    }

    std::string parse_word() {
        std::string str;

        while (input().peek() != EOF && ! isspace(input().peek())) {
            str.push_back(input().getc());
        }

        return str;
    }

    static const std::map<char, char>& escape_map() {
        static const std::map<char, char> escape_sequences = {
            {'a', '\a'},
            {'b', '\b'},
            {'e', '\e'},
            {'f', '\f'},
            {'n', '\n'},
            {'r', '\r'},
            {'t', '\t'},
            {'v', '\v'},
            {'\\', '\\'},
            {'\"', '\"'}
        };
        return escape_sequences;
    }

    static const std::regex& rx_unsafe() {
        static const std::regex unsafe = std::regex("[^\\w@%\\-+=:,./]", std::regex::ECMAScript);
        return unsafe;
    }

    static const std::regex& rx_quotes() {
        static const std::regex quotes = std::regex("(\'+)", std::regex::ECMAScript);
        return quotes;
    }

    std::set<char> _punctuation = {};
};

//-------------------------------------------------------------------
class ShellStringLexer : public ShellLexer {
public:
    ShellStringLexer(const std::string& str) : _infile(str), _input(_infile) { }

    file::BufferedInput& input() override {
        return _input;
    }

private:
    std::istringstream _infile;
    file::BufferedInput _input;
};

//-------------------------------------------------------------------
inline std::vector<std::string> split(const std::string& str) {
    auto lex = ShellStringLexer(str);
    std::vector<std::string> argv;
    for (auto iter = lex.begin(); iter != lex.end(); iter++) {
        argv.push_back(*iter);
    }
    return argv;
}

//-------------------------------------------------------------------
inline std::string quote(const std::string& str) {
    return ShellLexer::quote(str);
}

//-------------------------------------------------------------------
inline std::string join(const std::vector<std::string>& cmd) {
    return str::join(collect::map<std::string>(cmd, quote), " ");
}

}
}


#endif /* !__MOONLIGHT_SHLEX_H */
