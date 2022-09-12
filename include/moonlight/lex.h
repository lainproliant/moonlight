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
class LexParseError : public core::ValueError {
    using ValueError::ValueError;
};

// ------------------------------------------------------------------
class NoMatchError : public LexParseError {
public:
    NoMatchError(const Location& loc, char c, const std::vector<std::string>& gstack,
                 const debug::Source& srcloc)
    : _loc(loc), _chr(c), _gstack(gstack), LexParseError(format_message(loc, c), srcloc) { }

    const Location& loc() const {
        return _loc;
    }

    const std::vector<std::string>& gstack() const {
        return _gstack;
    }

    char chr() const {
        return _chr;
    }

private:
    static std::string format_message(const Location& loc, const char chr) {
        std::ostringstream sb;
        sb << "No lexical rules matched content starting at " << loc << " [" << str::literal(str::chr(chr)) << "].";
        return sb.str();
    }

    const Location& _loc;
    const std::vector<std::string> _gstack;
    const char _chr;
};

// ------------------------------------------------------------------
class UnexpectedEndOfContentError : public LexParseError {
public:
    UnexpectedEndOfContentError(const Location& loc, const debug::Source& srcloc)
    : _loc(loc), LexParseError(format_message(loc), srcloc) { }

    const Location& loc() const {
        return _loc;
    }

private:
    static std::string format_message(const Location& loc) {
        std::ostringstream sb;
        sb << "Parsing terminated early (at " << loc << ").";
        return sb.str();
    }

    const Location& _loc;
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

    const std::string str() const {
        return group();
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

        static ScanResult default_pop(const Location& loc);
        static ScanResult default_push(const Location& loc, Grammar::Pointer target);

        friend std::ostream& operator<<(std::ostream& out, const ScanResult& s);
    };

    static Pointer create() {
        return Pointer(new Grammar());
    }

    Pointer sub() {
        _sub_grammars.emplace_back(create_sub());
        return _sub_grammars.back();
    }

    const std::string& name() const {
        return _name;
    }

    static std::vector<std::string> gstack_to_strv(const std::stack<Grammar::Pointer>& gstack) {
        std::vector<std::string> result;
        std::vector<Grammar::Pointer> gvec;
        std::stack<Grammar::Pointer> stack_copy = gstack;

        while (! stack_copy.empty()) {
            gvec.push_back(stack_copy.top());
            stack_copy.pop();
        }

        std::transform(gvec.begin(), gvec.end(), std::back_inserter(result), [](auto g) {
            return g->name();
        });

        return result;
    }

    Pointer def(const Rule& rule);
    Pointer def(const Rule& rule, const std::string& type);
    Pointer named(const std::string& name);
    Pointer else_pop();
    Pointer else_push(Pointer target);

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
    Pointer _default_push_target = nullptr;
    std::vector<Pointer> _sub_grammars;
    const bool _sub_grammar;
    bool _default_pop = false;
    std::string _name = "?";
};

// ------------------------------------------------------------------
class Rule {
public:
    typedef std::shared_ptr<Rule> Pointer;

    Rule(Action action) : _action(action) { }

    static const Rule& default_pop();
    static const Rule& default_push(Grammar::Pointer target);

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

    Rule& icase() {
        _icase = true;
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

    bool advance() const {
        return _advance;
    }

    Rule& stay() {
        if (_action != Action::PUSH && _action != Action::POP) {
            THROW(core::UsageError, "stay() is only allowed on PUSH or POP actions.");
        }
        _advance = false;
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
    bool _icase = false;
    bool _advance = true;
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
inline Grammar::Pointer Grammar::named(const std::string& name) {
    _name = name;
    return shared_from_this();
}

// ------------------------------------------------------------------
inline Grammar::Pointer Grammar::else_pop() {
    _default_pop = true;
    return shared_from_this();
}

// ------------------------------------------------------------------
inline Grammar::Pointer Grammar::else_push(Grammar::Pointer target) {
    _default_push_target = target;
    return shared_from_this();
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

    if (_default_pop) {
        return ScanResult::default_pop(loc);
    }

    if (_default_push_target) {
        return ScanResult::default_push(loc, _default_push_target);
    }

    return {};
}

// ------------------------------------------------------------------
inline Rule define_default_pop() {
    static Rule pop = Rule(Action::POP);
    pop.type("default-pop");
    pop.stay();
    return pop;
}

// ------------------------------------------------------------------
inline const Rule& Rule::default_pop() {
    static Rule pop = []() {
        Rule pop = Rule(Action::POP);
        pop.type("default-pop");
        pop.stay();
        return pop;
    }();
    return pop;
}

// ------------------------------------------------------------------
inline const Rule& Rule::default_push(Grammar::Pointer target) {
    static std::map<Grammar::Pointer, Rule> push_rules_map;

    if (! push_rules_map.contains(target)) {
        push_rules_map.insert({target, [&]() {
            Rule push = Rule(Action::PUSH);
            push.target(target);
            push.type("default-push");
            push.stay();
            return push;
        }()});
    }

    return push_rules_map.at(target);
}

// ------------------------------------------------------------------
inline Grammar::ScanResult Grammar::ScanResult::default_pop(const Location& loc) {
    return {
        Rule::default_pop(),
        {},
        loc
    };
}

// ------------------------------------------------------------------
inline Grammar::ScanResult Grammar::ScanResult::default_push(const Location& loc, Grammar::Pointer target) {
    return {
        Rule::default_push(target),
        {},
        loc
    };
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
        _debug_print = true;

        try {
            auto tokens = lex(grammar, infile);

        } catch (const NoMatchError& e) {
            std::cout << "gstack --> " << str::join(e.gstack(), ",") << std::endl;
            throw e;
        }
    }

    std::vector<Token> lex(Grammar::Pointer grammar,
                           const std::string& content) const {
        std::vector<Token> tokens;
        std::stack<Grammar::Pointer> gstack;
        gstack.push(grammar);
        Location loc;

        auto append_token = [&](const Token& tk) {
            tokens.push_back(tk);
            if (_debug_print) {
                std::cout << tk << std::endl;
            }
        };

        while (loc.offset < content.size() && !gstack.empty()) {
            const Grammar::Pointer g = gstack.top();
            auto result_opt = g->scan(loc, content);

            if (! result_opt.has_value()) {
                if (_throw_on_error) {
                    THROW(NoMatchError, loc, content[loc.offset], Grammar::gstack_to_strv(gstack));
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
                append_token(result.token.value());
                break;

            case Action::POP:
                if (result.token.has_value()) {
                    append_token(result.token.value());
                }
                gstack.pop();
                break;

            case Action::PUSH:
                if (result.token.has_value()) {
                    append_token(result.token.value());
                }
                gstack.push(result.rule.target());
                break;
            }

            if (result.rule.advance()) {
                loc = result.loc;
            }
        }

        if (loc.offset < content.size() && _throw_on_error) {
            THROW(UnexpectedEndOfContentError, loc);
        }

        return tokens;
    }

    Lexer& throw_on_error(bool value) {
        _throw_on_error = value;
        return *this;
    }

private:
    bool _debug_print = false;
    bool _throw_on_error = true;
};

}
}

#endif /* !__LEX_H */
