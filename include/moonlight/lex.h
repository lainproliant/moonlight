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
#include <optional>
#include "moonlight/file.h"
#include "moonlight/exceptions.h"
#include "moonlight/string.h"
#include "moonlight/collect.h"

namespace moonlight {
namespace lex {

// ------------------------------------------------------------------
struct Location {
    unsigned int line = 1;
    unsigned int col = 1;
    unsigned int offset = 0;

    static Location nowhere() {
        return {
            0, 0, 0
        };
    }

    friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
        out << "<line " << loc.line << ", col " << loc.col << ", offset " << loc.offset << ">";
        return out;
    }
};

// ------------------------------------------------------------------
enum class Action {
    IGNORE,
    MATCH,
    POP,
    PUSH,
};

// ------------------------------------------------------------------
inline const std::string& _action_name(Action action) {
    static const std::map<Action, std::string> action_names = {
        {Action::IGNORE, "IGNORE"},
        {Action::MATCH, "MATCH"},
        {Action::PUSH, "PUSH"},
        {Action::POP, "POP"}
    };
    return action_names.at(action);
}

// ------------------------------------------------------------------
class Match {
public:
    Match(const Location& location, const std::smatch& smatch)
    : _location(location), _length(smatch.length()), _groups(smatch.begin(), smatch.end()) { }

    Match(const Location& location)
    : _location(location), _length(0) { }

    const Location& location() const {
        return _location;
    }

    unsigned int length() const {
        return _length;
    }

    const std::string& group(size_t offset = 0) const {
        return _groups[offset];
    }

    const std::vector<std::string>& groups() const {
        return _groups;
    }

    friend std::ostream& operator<<(std::ostream& out, const Match& match) {
        auto literal_matches = collect::map<std::string>(match.groups(), _literalize);
        out << "Match<[" << str::join(literal_matches, ",") << "], "
            << match.length() << "c @ " << match.location() << ">";
        return out;
    }

private:
    static std::string _literalize(const std::string& s) {
        std::ostringstream sb;
        sb << "\"" << str::literal(s) << "\"";
        return sb.str();
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

    static Token nothing() {
        return Token("NOTHING", Match(Location::nowhere()));
    }

    const std::string& type() const {
        return _type;
    }

    const Match& match() const {
        return _match;
    }

    friend std::ostream& operator<<(std::ostream& out, const Token& tk) {
        out << "<" << tk.type() << " " << tk.match() << ">";
        return out;
    }

private:
    const std::string _type;
    const Match _match;
};

// ------------------------------------------------------------------
class Rule;
typedef std::shared_ptr<std::vector<Rule>> RuleContainer;

// ------------------------------------------------------------------
class Grammar : public std::enable_shared_from_this<Grammar> {
public:
    typedef std::shared_ptr<Grammar> Pointer;

    struct ScanResult {
        const Rule& rule;
        std::optional<Token> token;
        const Location loc;

        friend std::ostream& operator<<(std::ostream& out, const ScanResult& s);
    };

    static Pointer create() {
        return Pointer(new Grammar());
    }

    Pointer sub() {
        _sub_grammars.emplace_back(create_sub());
        return _sub_grammars.back();
    }

    Pointer def(const Rule& rule);
    Pointer def(const Rule& rule, const std::string& type);

    Pointer inherit(Pointer super);

    std::optional<ScanResult> scan(Location loc, const std::string& content) const;

private:
    Grammar(bool sub_grammar) : _sub_grammar(sub_grammar) { }
    Grammar() : _sub_grammar(false) { }

    static Pointer create_sub() {
        return Pointer(new Grammar(true));
    }

    bool is_sub_grammar() const {
        return _sub_grammar;
    }

    void add_rule(const Rule& rule);

    std::vector<Pointer> _parents;
    RuleContainer _rules = nullptr;
    std::vector<Pointer> _sub_grammars;
    const bool _sub_grammar;
};

// ------------------------------------------------------------------
class Rule {
public:
    typedef std::shared_ptr<Rule> Pointer;

    Rule(Action action) : _action(action) { }

    Action action() const {
        return _action;
    }

    const std::string& action_name() const {
        return _action_name(action());
    }

    const std::regex& rx() const {
        return _rx;
    }

    const std::string& rx_str() const {
        return _rx_str;
    }

    Rule& rx(const std::string& rx) {
        _rx_str = rx;
        compile_regex();
        return *this;
    }

    Rule& icase(bool icase = true) {
        _icase = icase;
        compile_regex();
        return *this;
    }

    const std::string& type() const {
        return _type;
    }

    Rule& type(const std::string& type) {
        _type = type;
        return *this;
    }

    Grammar::Pointer target() const {
        if (_target == nullptr) {
            std::ostringstream sb;
            sb << "Rule type " << type() << " has no subgrammar target.";
            THROW(core::UsageError, sb.str());
        }
        return _target;
    }

    Rule& target(Grammar::Pointer target) {
        _target = target;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& out, const Rule& r) {
        out << "Rule<";
        out << (r.type() == "" ? "(no name)" : r.type()) << " ";
        out << r.action_name() << " ";
        out << str::literal(r.rx_str());
        if (r._icase) {
            out << "i";
        }
        out << ">";
        return out;
    }

private:
    void compile_regex() {
        if (_icase) {
            _rx = std::regex("^" + _rx_str, std::regex_constants::ECMAScript | std::regex_constants::icase);
        } else {
            _rx = std::regex("^" + _rx_str, std::regex_constants::ECMAScript);
        }
    }

    const Action _action;
    bool _icase;
    std::regex _rx;
    std::string _rx_str;
    std::string _type;
    Grammar::Pointer _target = nullptr;
};

// ------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& out, const Grammar::ScanResult& s) {
    out << "ScanResult<" << s.rule << ", "
        << s.token.value_or(Token::nothing()) << ", " << s.loc << ">";
    return out;
}

// ------------------------------------------------------------------
inline Rule ignore(const std::string& rx) {
    return Rule(Action::IGNORE).rx(rx);
}

inline Rule match(const std::string& rx) {
    return Rule(Action::MATCH).rx(rx);
}

inline Rule push(const std::string& rx, Grammar::Pointer target) {
    return Rule(Action::PUSH).rx(rx).target(target);
}

inline Rule pop(const std::string& rx) {
    return Rule(Action::POP).rx(rx);
}

inline Rule copy_rule(const Rule& rule) {
    return Rule(rule.action()).rx(rule.rx_str()).target(rule.target());
}

// ------------------------------------------------------------------
inline void Grammar::add_rule(const Rule& rule) {
    if (_rules == nullptr) {
        _rules = std::make_shared<std::vector<Rule>>();
    }
    _rules->emplace_back(rule);
}

// ------------------------------------------------------------------
inline Grammar::Pointer Grammar::def(const Rule& rule) {
    add_rule(rule);
    return shared_from_this();
}

// ------------------------------------------------------------------
inline Grammar::Pointer Grammar::def(const Rule& rule, const std::string& type) {
    Rule rule_copy = rule;
    rule_copy.type(type);
    return def(rule_copy);
}

// ------------------------------------------------------------------
inline Grammar::Pointer Grammar::inherit(Grammar::Pointer super) {
    _parents.push_back(super);
    return shared_from_this();
}

// ------------------------------------------------------------------
inline std::optional<Grammar::ScanResult> Grammar::scan(Location loc, const std::string& content) const {
    for (const auto& rule : *_rules) {
        std::smatch smatch = {};
        if (std::regex_search(content.begin() + loc.offset, content.end(), smatch, rule.rx())) {
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

            if (! rule.type().empty()) {
                return ScanResult{rule, Token(rule.type(), match), loc};
            } else {
                return ScanResult{rule, {}, loc};
            }
        }
    }

    for (const auto& parent : _parents) {
        const auto parent_result = parent->scan(loc, content);
        if (parent_result.has_value()) {
            return parent_result;
        }
    }

    return {};
}

// ------------------------------------------------------------------
class Lexer {
public:
    std::vector<Token> lex(Grammar::Pointer grammar,
                           std::istream& infile) const {
        return lex(grammar, file::to_string(infile));
    }

    void debug_print_tokens(Grammar::Pointer grammar,
                            std::istream& infile = std::cin) {
        auto tokens = lex(grammar, infile);
        for (auto& token : tokens) {
            std::cout << token.type() << ": " << str::literal(token.match().group()) << std::endl;
        }
    }

    std::vector<Token> lex(Grammar::Pointer grammar,
                           const std::string& content) const {
        std::vector<Token> tokens;
        std::stack<Grammar::Pointer> gstack;
        gstack.push(grammar);
        Location loc;

        while (loc.offset < content.size() && !gstack.empty()) {
            const Grammar::Pointer g = gstack.top();
            auto result_opt = g->scan(loc, content);

            if (! result_opt.has_value()) {
                std::cout << "LRS-DEBUG: result_opt.has_value() == false" << std::endl;
                if (_throw_on_error) {
                    std::ostringstream sb;
                    sb << "No lexical rules matched content starting at " << loc << ".";
                    THROW(core::ValueError, sb.str());
                } else {
                    break;
                }
            }

            auto result = result_opt.value();

            switch(result.rule.action()) {
            case Action::IGNORE:
                break;

            case Action::MATCH:
                if (! result.token.has_value()) {
                    std::ostringstream sb;
                    sb << "Match rule " << result.rule.type()
                       << " didn't yield a token (at " << loc << ").";
                    THROW(core::UsageError, sb.str());
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

        if (loc.offset < content.size() && _throw_on_error) {
            std::ostringstream sb;
            sb << "Parsing terminated early (at " << loc << ").";
            THROW(core::ValueError, sb.str());
        }

        return tokens;
    }

    Lexer& throw_on_error(bool value) {
        _throw_on_error = value;
        return *this;
    }

private:
    bool _throw_on_error = true;
};

}
}

#endif /* !__LEX_H */
