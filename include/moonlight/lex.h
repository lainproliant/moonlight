/*
 * lex.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday January 30, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_LEX_H
#define __MOONLIGHT_LEX_H

#include <optional>
#include <algorithm>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include <stack>

#include "moonlight/file.h"
#include "moonlight/exceptions.h"
#include "moonlight/string.h"
#include "moonlight/rx.h"

namespace moonlight {
namespace lex {

// ------------------------------------------------------------------
class LexParseError : public core::ValueError {
    using ValueError::ValueError;
};

// ------------------------------------------------------------------
class NoMatchError : public LexParseError {
 public:
     NoMatchError(const file::Location& loc, char c, const std::vector<std::string>& gstack,
                  const debug::Source& srcloc)
     : LexParseError(format_message(loc, c), srcloc), _loc(loc), _gstack(gstack), _chr(c) { }

     const file::Location& loc() const {
         return _loc;
     }

     const std::vector<std::string>& gstack() const {
         return _gstack;
     }

     char chr() const {
         return _chr;
     }

 private:
     static std::string format_message(const file::Location& loc, const char chr) {
         std::ostringstream sb;
         sb << "No lexical rules matched content starting at " << loc << " [" << str::literal(str::chr(chr)) << "].";
         return sb.str();
     }

     const file::Location& _loc;
     const std::vector<std::string> _gstack;
     const char _chr;
};

// ------------------------------------------------------------------
class UnexpectedEndOfContentError : public LexParseError {
 public:
     UnexpectedEndOfContentError(const file::Location& loc, const debug::Source& srcloc)
     : LexParseError(format_message(loc), srcloc), _loc(loc) { }

     const file::Location& loc() const {
         return _loc;
     }

 private:
     static std::string format_message(const file::Location& loc) {
         std::ostringstream sb;
         sb << "Parsing terminated early (at " << loc << ").";
         return sb.str();
     }

     const file::Location& _loc;
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
template<class T>
class Token {
 public:
     Token(const T& type, const rx::Capture& capture, const file::Location loc = file::Location::nowhere())
     : _type(type), _capture(capture), _loc(loc) { }

     static Token nothing() {
         return Token("NOTHING", rx::Capture());
     }

     const T& type() const {
         return _type;
     }

     const rx::Capture& capture() const {
         return _capture;
     }

     const file::Location& loc() const {
         return _loc;
     }

     friend std::ostream& operator<<(std::ostream& out, const Token<T>& tk) {
         out << "<" << tk.type() << " " << tk.capture() << ">";
         return out;
     }

 private:
     const T _type;
     const rx::Capture _capture;
     const file::Location _loc;
};

// ------------------------------------------------------------------
class Rule;

template<class T>
class QualifiedRule;

template<class T>
using RuleContainer = std::shared_ptr<std::vector<QualifiedRule<T>>>;

template<class T>
class Lexer;

using Location = file::Location;

// ------------------------------------------------------------------
template<class T>
class GrammarImpl : public std::enable_shared_from_this<GrammarImpl<T>> {
 public:
     typedef std::shared_ptr<GrammarImpl<T>> Pointer;
     typedef std::shared_ptr<const GrammarImpl<T>> ConstPointer;
     typedef Lexer<T> Lexer;
     typedef Token<T> Token;

     struct ScanResult {
         const QualifiedRule<T> rule;
         std::optional<Token> token;
         file::Location loc;

         static ScanResult default_pop(const Location& loc);
         static ScanResult default_push(const Location& loc, GrammarImpl::Pointer target);

         friend std::ostream& operator<<(std::ostream& out, const GrammarImpl<T>::ScanResult& s) {
             out << "ScanResult<" << s.rule << ", "
             << s.token.value_or(Token::nothing()) << ">";
             return out;
         }
     };

     static Pointer create() {
         return Pointer(new GrammarImpl());
     }

     Pointer sub() {
         _sub_grammars.emplace_back(create_sub());
         return _sub_grammars.back();
     }

     const std::string& name() const {
         return _name;
     }

     static std::vector<std::string> gstack_to_strv(const std::stack<ConstPointer>& gstack) {
         std::vector<std::string> result;
         std::vector<ConstPointer> gvec;
         std::stack<ConstPointer> stack_copy = gstack;

         while (! stack_copy.empty()) {
             gvec.push_back(stack_copy.top());
             stack_copy.pop();
         }

         std::transform(gvec.begin(), gvec.end(), std::back_inserter(result), [](auto g) {
             return g->name();
         });

         return result;
     }

     Pointer def(const QualifiedRule<T>& rule);
     Pointer def(const Rule& rule);
     Pointer def(const Rule& rule, const T& type);
     Pointer named(const std::string& name);
     Pointer else_pop();
     Pointer else_push(Pointer target);

     Pointer inherit(Pointer super);

     std::optional<ScanResult> scan(file::Location loc, const std::string& content) const;

 private:
     explicit GrammarImpl(bool sub_grammar) : _sub_grammar(sub_grammar) { }
     GrammarImpl() : _sub_grammar(false) { }

     static Pointer create_sub() {
         return Pointer(new GrammarImpl(true));
     }

     bool is_sub_grammar() const {
         return _sub_grammar;
     }

     void add_rule(const QualifiedRule<T>& rule);

     std::vector<Pointer> _parents;
     RuleContainer<T> _rules = nullptr;
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

     Rule() : _action(Action::IGNORE) { }
     Rule(Action action) : _action(action) { }
     Rule(const Rule& rule)
     : _action(rule._action), _icase(rule._icase), _advance(rule._advance), _rx(rule._rx), _rx_str(rule._rx_str), _target(rule._target) { }

     static Rule default_pop();
     static Rule default_push(std::shared_ptr<void> target);

     Action action() const {
         return _action;
     }

     const std::string& action_name() const {
         return _action_name(action());
     }

     const rx::Expression& rx() const {
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

     template<class T>
     GrammarImpl<T>::ConstPointer target() const {
         if (_target == nullptr) {
             std::ostringstream sb;
             sb << "Rule type has no subgrammar target.";
             THROW(core::UsageError, sb.str());
         }
         return _target;
     }

     std::shared_ptr<const void> raw_target() const {
         return _target;
     }

     Rule& target(std::shared_ptr<const void> target) {
         _target = target;
         return *this;
     }

     friend std::ostream& operator<<(std::ostream& out, const Rule& r) {
         out << "Rule<";
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
             _rx = rx::idef("^" + _rx_str);
         } else {
             _rx = rx::def("^" + _rx_str);
         }
     }

     Action _action;
     bool _icase = false;
     bool _advance = true;
     rx::Expression _rx;
     std::string _rx_str;
     std::shared_ptr<const void> _target = nullptr;
};

// ------------------------------------------------------------------
template<class T>
class QualifiedRule : public Rule {
 public:
     QualifiedRule(const Rule& rule)
     : Rule(rule), _type() { }

     QualifiedRule(const Rule& rule, const T& type)
     : Rule(rule), _type(type) { }

     const T& type() const {
         return _type.value();
     }

     bool is_typeless() const {
         return ! _type.has_value();
     }

     QualifiedRule& type(const T& type) {
         _type = type;
         return *this;
     }

     friend std::ostream& operator<<(std::ostream& out, const QualifiedRule<T>& r) {
         out << "QualifiedRule<";
         out << r.type() << " ";
         out << r.action_name() << " ";
         out << str::literal(r.rx_str());
         if (r._icase) {
             out << "i";
         }
         out << ">";
         return out;
     }
 private:
     const std::optional<T> _type;
};

// ------------------------------------------------------------------
inline Rule ignore(const std::string& rx) {
    return Rule(Action::IGNORE).rx(rx);
}

inline Rule match(const std::string& rx) {
    return Rule(Action::MATCH).rx(rx);
}

inline Rule push(const std::string& rx, std::shared_ptr<void> target) {
    return Rule(Action::PUSH).rx(rx).target(target);
}

inline Rule pop(const std::string& rx) {
    return Rule(Action::POP).rx(rx);
}

inline Rule copy_rule(const Rule& rule) {
    return Rule(rule.action()).rx(rule.rx_str()).target(rule.raw_target());
}

// ------------------------------------------------------------------
template<class T>
inline void GrammarImpl<T>::add_rule(const QualifiedRule<T>& rule) {
    if (_rules == nullptr) {
        _rules = std::make_shared<std::vector<QualifiedRule<T>>>();
    }
    _rules->emplace_back(rule);
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::def(const QualifiedRule<T>& rule) {
    add_rule(rule);
    return this->shared_from_this();
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::def(const Rule& rule) {
    return def(QualifiedRule<T>(rule));
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::def(const Rule& rule, const T& type) {
    return def(QualifiedRule<T>(rule, type));
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::named(const std::string& name) {
    _name = name;
    return this->shared_from_this();
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::else_pop() {
    _default_pop = true;
    return this->shared_from_this();
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::else_push(GrammarImpl<T>::Pointer target) {
    _default_push_target = target;
    return this->shared_from_this();
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::Pointer GrammarImpl<T>::inherit(GrammarImpl<T>::Pointer super) {
    _parents.push_back(super);
    return this->shared_from_this();
}

// ------------------------------------------------------------------
template<class T>
inline std::optional<typename GrammarImpl<T>::ScanResult> GrammarImpl<T>::scan(Location loc, const std::string& content) const {
    for (const auto& rule : *_rules) {
        auto capture = rx::capture(rule.rx(), content.begin() + loc.offset, content.end());

        if (capture) {
            for (unsigned int x = 0; x < capture.length(); x++) {
                loc.offset++;
                if (*(content.begin() + loc.offset) == '\n') {
                    loc.line++;
                    loc.col = 1;
                } else {
                    loc.col++;
                }
            }

            if (! rule.is_typeless()) {
                return ScanResult{rule, Token(rule.type(), capture, loc), loc};
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
inline Rule Rule::default_pop() {
    Rule pop = Rule(Action::POP);
    pop.stay();
    return pop;
}

// ------------------------------------------------------------------
inline Rule Rule::default_push(std::shared_ptr<void> target) {
    Rule rule = Rule(Action::PUSH);
    rule.target(target);
    rule.stay();
    return rule;
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::ScanResult GrammarImpl<T>::ScanResult::default_pop(const Location& loc) {
    return {
        QualifiedRule<T>::default_pop(),
        {},
        loc
    };
}

// ------------------------------------------------------------------
template<class T>
inline GrammarImpl<T>::ScanResult GrammarImpl<T>::ScanResult::default_push(const Location& loc, GrammarImpl::Pointer target) {
    return {
        QualifiedRule<T>::default_push(target),
        {},
        loc
    };
}

// ------------------------------------------------------------------
template<class T>
class Grammar {
 public:
    typedef GrammarImpl<T>::ConstPointer ConstPointer;
    typedef GrammarImpl<T>::ScanResult ScanResult;
    typedef GrammarImpl<T>::Lexer Lexer;
    typedef GrammarImpl<T>::Token Token;

    Grammar() : _grammar(GrammarImpl<T>::create()) { }
    Grammar(const Grammar& other) : _grammar(other._grammar) { }

    const std::string& name() const {
        return _grammar->name();
    }

    Grammar sub() const {
        return Grammar(_grammar);
    }

    Grammar& def(const QualifiedRule<T>& rule) {
        _grammar->def(rule);
        return *this;
    }

    Grammar& def(const Rule& rule) {
        _grammar->def(rule);
        return *this;
    }

    Grammar& def(const Rule& rule, const T& type) {
        _grammar->def(rule, type);
        return *this;

    }

    Grammar& named(const std::string& name) {
        _grammar->named(name);
        return *this;
    }

    Grammar& else_pop() {
        _grammar->else_pop();
        return *this;
    }

    Grammar& else_push(const Grammar& other) {
        _grammar->else_push(other._grammar);
        return *this;
    }

    Grammar& inherit(const Grammar& other) {
        _grammar->inherit(other._grammar);
        return *this;
    }

    std::optional<ScanResult> scan(Location loc, const std::string& content) const {
        return _grammar.scan(loc, content);
    }

    ConstPointer pointer() const {
        return _grammar;
    }

    Lexer lexer() const {
        return Lexer(*this);
    }

private:
    Grammar(GrammarImpl<T>::Pointer _other) : _grammar(_other->sub()) { }

    GrammarImpl<T>::Pointer _grammar;
};


// ------------------------------------------------------------------
template<class T>
class Lexer {
 public:
     friend class Grammar<T>;

     std::vector<Token<T>> lex(std::istream& infile) const {
         return lex(file::to_string(infile));
     }

     void debug_print_tokens(std::istream& infile = std::cin) {
         _debug_print = true;

         try {
             auto tokens = lex(infile);
         } catch (const NoMatchError& e) {
             std::cout << "gstack --> " << str::join(e.gstack(), ",") << std::endl;
             throw e;
         }
     }

     std::vector<Token<T>> lex(const std::string& content) const {
         std::vector<Token<T>> tokens;
         std::stack<typename Grammar<T>::ConstPointer> gstack;
         gstack.push(_grammar.pointer());
         file::Location loc;

         auto append_token = [&](const Token<T>& tk) {
             tokens.push_back(tk);
             if (_debug_print) {
                 std::cout << tk << std::endl;
             }
         };

         while (loc.offset < content.size() && !gstack.empty()) {
             const typename Grammar<T>::ConstPointer g = gstack.top();
             auto result_opt = g->scan(loc, content);

             if (! result_opt.has_value()) {
                 if (_throw_on_error) {
                     THROW(NoMatchError, loc, content[loc.offset], GrammarImpl<T>::gstack_to_strv(gstack));
                 } else {
                     break;
                 }
             }

             auto result = result_opt.value();

             switch (result.rule.action()) {
             case Action::IGNORE:
                 break;

             case Action::MATCH:
                 if (result.token.has_value()) {
                     append_token(result.token.value());
                 } else if (! result.rule.is_typeless()) {
                     std::ostringstream sb;
                     sb << "Match rule " << result.rule.type()
                     << " didn't yield a token (at " << loc << ").";
                     THROW(core::UsageError, sb.str());
                 }
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
                 gstack.push(std::static_pointer_cast<const GrammarImpl<T>>(result.rule.raw_target()));
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
     explicit Lexer(const Grammar<T>& grammar)
     : _grammar(grammar) { }

     const Grammar<T> _grammar;
     bool _debug_print = false;
     bool _throw_on_error = true;
};

template<class T>
inline Rule push(const std::string& rx, const Grammar<T>& grammar) {
    return Rule(Action::PUSH).rx(rx).target(grammar.pointer());
}

}  // namespace lex
}  // namespace moonlight

#endif /* !__MOONLIGHT_LEX_H */
