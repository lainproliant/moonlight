/*
 * lex.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday January 30, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __LEX_H
#define __LEX_H

#include <regex>
#include "tinyformat/tinyformat.h"
#include "moonlight/core.h"

namespace moonlight {
namespace lex {

// ------------------------------------------------------------------
struct Location {
    unsigned int line = 1;
    unsigned int col = 1;
    unsigned int offset = 0;

    friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
        tfm::format(out, "<line %d, col %d, offset %d>", loc.line, loc.col, loc.offset);
        return out;
    }
};

// ------------------------------------------------------------------
class LexError : public core::Exception {
public:
    using Exception::Exception;
};

// ------------------------------------------------------------------
enum class Action {
    IGNORE,
    MATCH,
    POP,
    PUSH,
};

// ------------------------------------------------------------------
template<class G>
class Rule {
public:
    Rule(Action action, const std::string& rx,
         const std::string& type = "", const G* target = nullptr) :
    _action(action), _rx(rx), _type(type), _target(target) {
        if (target != nullptr && ! target->isSubGrammar()) {
            throw LexError("Push target must be a subgrammar of another grammar.");
        }
    }

    Rule(Rule&&) = default;

    Action action() const {
        return _action;
    }

    const std::regex& rx() const {
        return _rx;
    }

    const std::string& type() const {
        return _type;
    }

    const G* target() const {
        if (_target == nullptr) {
            throw LexError(tfm::format("Rule type %s has no subgrammar target.",
                                       type()));
        }
        return _target;
    }

private:
    const Action _action;
    const std::regex _rx;
    const std::string _type;
    const G* _target = nullptr;
};

// ------------------------------------------------------------------
class Token {
public:
    Token(const std::string& type, const std::smatch& smatch,
          const Location& location) :
    _type(type), _smatch(smatch), _location(location) { }

    const std::string& type() const {
        return _type;
    }

    const Location& location() const {
        return _location;
    }

    std::string match(unsigned int group = 0) const {
        if (_smatch.size() <= group) {
            throw LexError(tfm::format("Token type '%d' has no group at offset %d.",
                                       type(), group));
        }
        return _smatch[group];
    }

    friend std::ostream& operator<<(std::ostream& out, const Token& tk) {
        tfm::format("<%s@%s (%s)>", tk.type(), tk.location(), str::join(tk._smatch, ","));
        return out;
    }

private:
    const std::string _type;
    const std::smatch _smatch;
    const Location _location;
};

// ------------------------------------------------------------------
class Grammar {
public:
    typedef Rule<Grammar> GrammarRule;

    Grammar() : _subGrammar(false) { }
    ~Grammar() {
        for (auto g : _subGrammars) {
            delete g;
        }
    }

    struct ScanResult {
        const GrammarRule& rule;
        std::optional<Token> token;
        const Location loc;
    };

    bool isSubGrammar() const {
        return _subGrammar;
    }

    Grammar& def(Action action, const std::string& rx,
                 const std::string& type = "",
                 std::optional<const Grammar> target = {}) {
        if (target.has_value()) {
            _rules.push_back(std::make_shared<GrammarRule>(action, rx, type, &target.value()));
        } else {
            _rules.push_back(std::make_shared<GrammarRule>(action, rx, type));
        }
        return *this;
    }

    Grammar& ignore(const std::string& rx) {
        return def(Action::IGNORE, rx);
    }

    Grammar& match(const std::string& rx, const std::string& type = "") {
        return def(Action::MATCH, rx, type);
    }

    Grammar& pop(const std::string& rx, const std::string& type = "") {
        return def(Action::POP, rx, type);
    }

    Grammar& push(const std::string& rx, const Grammar& grammar,
                  const std::string& type = "") {
        return def(Action::PUSH, rx, type, grammar);
    }

    Grammar& sub() {
        _subGrammars.emplace_back(new Grammar(true));
        return *_subGrammars.back();
    }

    ScanResult scan(Location loc, const std::string& content) const {
        for (auto& rule : _rules) {
            std::smatch smatch = {};
            if (std::regex_match(content.begin() + loc.offset, content.end(), smatch, rule->rx())) {
                for (unsigned int x = 0; x < smatch.size(); x++) {
                    loc.offset++;
                    if (*(content.begin() + loc.offset) == '\n') {
                        loc.line++;
                        loc.col = 1;
                    } else {
                        loc.col++;
                    }
                }

                if (! rule->type().empty()) {
                    return {*rule, Token(rule->type(), smatch, loc), loc};
                } else {
                    return {*rule, {}, loc};
                }
            }
        }

        throw LexError(tfm::format("No lexical rules matched content starting at %s.", loc));
    }

private:
    Grammar(bool subGrammar) : _subGrammar(subGrammar) { }

    std::vector<std::shared_ptr<const GrammarRule>> _rules;
    std::vector<Grammar*> _subGrammars;
    const bool _subGrammar;
};

// ------------------------------------------------------------------
class Lexer {
public:
    std::vector<Token> lex(const Grammar& grammar,
                           const std::string& content) const {
        std::vector<Token> tokens;
        std::stack<const Grammar*> gstack;
        gstack.push(&grammar);
        Location loc;

        while (loc.offset < content.size()) {
            const Grammar& g = *gstack.top();
            auto result = g.scan(loc, content);

            switch(result.rule.action()) {
            case Action::IGNORE:
                break;

            case Action::MATCH:
                if (! result.token.has_value()) {
                    throw LexError(tfm::format("Match rule '%s' didn't yield a token (at %s)",
                                               result.rule.type(), loc));
                }
                tokens.push_back(result.token.value());
                break;

            case Action::POP:
                if (result.token.has_value()) {
                    tokens.push_back(result.token.value());
                }
                gstack.pop();
                break;

            case Action::PUSH:
                if (result.token.has_value()) {
                    tokens.push_back(result.token.value());
                }
                gstack.push(result.rule.target());
                break;
            }

            loc = result.loc;
        }

        return tokens;
    }
};

}
}

#endif /* !__LEX_H */
