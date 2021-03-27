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
#include "moonlight/collect.h"

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
    typedef std::shared_ptr<Rule<G>> Pointer;

    Rule(Action action, const std::string& rx,
         const std::string& type = "", typename G::Pointer target = nullptr) :
    _action(action), _rx("^" + rx), _type(type), _target(target) {
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

    const typename G::Pointer target() const {
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
    const typename G::Pointer _target = nullptr;
};

// ------------------------------------------------------------------
class Match {
public:
    Match(const Location& location, const std::smatch& smatch)
    : _location(location), _length(smatch.length()), _groups(smatch.begin(), smatch.end()) { }

    const Location& location() const {
        return _location;
    }

    unsigned int length() const {
        return _length;
    }

    const std::vector<std::string>& groups() const {
        return _groups;
    }

    friend std::ostream& operator<<(std::ostream& out, const Match& match) {
        auto literal_matches = collect::map<std::string>(match.groups(), _literalize);
        tfm::format(out, "Match<%s, %dc @ %s>",
                    str::join(literal_matches, ","),
                    match.length(),
                    match.location());
        return out;
    }

private:
    static std::string _literalize(const std::string& s) {
        return tfm::format("\"%s\"", str::literal(s));
    }

    const Location _location;
    unsigned int _length;
    const std::vector<std::string> _groups;
};

// ------------------------------------------------------------------
class Token {
public:
    Token(const std::string& type, const Match& smatch)
    : _type(type), _match(smatch) { }

    const std::string& type() const {
        return _type;
    }

    const Match& match() const {
        return _match;
    }

    friend std::ostream& operator<<(std::ostream& out, const Token& tk) {
        tfm::format(out, "<%s %s>", tk.type(), tk.match());
        return out;
    }

private:
    const std::string _type;
    const Match _match;
};

// ------------------------------------------------------------------
class Grammar : public std::enable_shared_from_this<Grammar> {
public:
    typedef Rule<Grammar> GrammarRule;
    typedef std::shared_ptr<Grammar> Pointer;

    struct ScanResult {
        const GrammarRule& rule;
        std::optional<Token> token;
        const Location loc;
    };

    static Pointer create() {
        return Pointer(new Grammar());
    }

    bool isSubGrammar() const {
        return _subGrammar;
    }

    Pointer def(Action action, const std::string& rx,
                 const std::string& type = "",
                 std::optional<Pointer> target = {}) {
        if (target.has_value()) {
            _rules.push_back(std::make_shared<GrammarRule>(action, rx, type, target.value()));
        } else {
            _rules.push_back(std::make_shared<GrammarRule>(action, rx, type));
        }
        return shared_from_this();
    }

    Pointer ignore(const std::string& rx) {
        return def(Action::IGNORE, rx);
    }

    Pointer match(const std::string& rx, const std::string& type = "") {
        return def(Action::MATCH, rx, type);
    }

    Pointer pop(const std::string& rx, const std::string& type = "") {
        return def(Action::POP, rx, type);
    }

    Pointer push(const std::string& rx, Pointer grammar,
                 const std::string& type = "") {
        return def(Action::PUSH, rx, type, grammar);
    }

    Pointer sub() {
        _subGrammars.emplace_back(create_sub());
        return _subGrammars.back();
    }

    ScanResult scan(Location loc, const std::string& content) const {
        for (auto& rule : _rules) {
            std::smatch smatch = {};
            if (std::regex_search(content.begin() + loc.offset, content.end(), smatch, rule->rx())) {
                Match match(loc, smatch);

                for (unsigned int x = 0; x < smatch.length(); x++) {
                    loc.offset++;
                    if (*(content.begin() + loc.offset) == '\n') {
                        loc.line++;
                        loc.col = 1;
                    } else {
                        loc.col++;
                    }
                }

                if (! rule->type().empty()) {
                    return {*rule, Token(rule->type(), match), loc};
                } else {
                    return {*rule, {}, loc};
                }
            }
        }

        throw LexError(tfm::format("No lexical rules matched content starting at %s.", loc));
    }

private:
    Grammar(bool subGrammar) : _subGrammar(subGrammar) { }
    Grammar() : _subGrammar(false) { }

    static Pointer create_sub() {
        return Pointer(new Grammar(true));
    }

    std::vector<GrammarRule::Pointer> _rules;
    std::vector<Pointer> _subGrammars;
    const bool _subGrammar;
};

// ------------------------------------------------------------------
class Lexer {
public:
    std::vector<Token> lex(Grammar::Pointer grammar,
                           const std::string& content) const {
        std::vector<Token> tokens;
        std::stack<Grammar::Pointer> gstack;
        gstack.push(grammar);
        Location loc;

        while (loc.offset < content.size()) {
            const Grammar::Pointer g = gstack.top();
            auto result = g->scan(loc, content);

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
