/*
 * parser.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_PARSER_H
#define __MOONLIGHT_JSON_PARSER_H

#include "moonlight/json/core.h"
#include "moonlight/json/object.h"
#include "moonlight/json/array.h"
#include "moonlight/file.h"
#include "moonlight/automata.h"
#include "moonlight/collect.h"

#include <iostream>

namespace moonlight {
namespace json {

namespace parser {

//-------------------------------------------------------------------
struct Location {
    std::string name;
    int line;
    int col;

    friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
        out << loc.name
            << ", line " << loc.line
            << ", col " << loc.col;
        return out;
    }
};

//-------------------------------------------------------------------
class ParseError : public moonlight::core::RuntimeError {
public:
    ParseError(const std::string& msg, const Location& loc, debug::Source where = {}) :
    moonlight::core::RuntimeError(format_message(msg, loc), where, type_name<ParseError>()), _loc(loc) { }

    static std::string format_message(const std::string& msg,
                                      const Location& loc) {
        std::ostringstream sb;
        sb << msg << " (" << loc << ")";
        return sb.str();
    }

    const Location& loc() const {
        return _loc;
    }

private:
    Location _loc;
};

//-------------------------------------------------------------------
struct Context {
    file::BufferedInput input;

    Location loc() const {
        return {
            input.name(), input.line(), input.col()
        };
    }

    std::string dbg_cursor() {
        std::ostringstream sb;
        sb << "\"";
        std::string cursor;
        for (int x = 1; x <= 70 && input.peek(x) != EOF; x++) {
            cursor.push_back(input.peek(x));
        }
        sb << str::literal(cursor) << "\"";
        return sb.str();
    }
};

//-------------------------------------------------------------------
class State : public automata::State<Context> {
protected:
    bool is_double_char(int c) {
        static const std::set<char> DOUBLE_CHARS = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            '-', '+', '.', 'e', 'E'};
        return DOUBLE_CHARS.find(c) != DOUBLE_CHARS.end();
    }

    double parse_double() {
        std::string double_str;

        for (;;) {
            int c = peek();
            if (is_double_char(c)) {
                double_str.push_back(c);
                advance();
            } else {
                break;
            }
        }

        size_t pos;
        double result = std::stod(double_str, &pos);
        if (pos != double_str.size()) {
            THROW(ParseError, "Malformed double precision value.", context().loc());
        }

        return result;
    }

    std::string parse_literal() {
        static const std::map<char, char> ESCAPED_CHARS = {
            {'a', '\a'},
            {'b', '\b'},
            {'e', '\e'},
            {'f', '\f'},
            {'n', '\n'},
            {'r', '\r'},
            {'t', '\t'},
            {'v', '\v'},
            {'"', '"'},
            {'\\', '\\'}
        };
        std::string result;

        int c = getc();
        if (c != '"') {
            THROW(ParseError, "Input is not a string literal.", context().loc());
        }

        while ((c = getc()) != '"') {
            if (c == '\\') {
                int c2 = getc();
                auto iter = ESCAPED_CHARS.find(c2);
                if (iter != ESCAPED_CHARS.end()) {
                    result.push_back(iter->second);

                } else if (c2 == 'x') {
                    int hexA = getc();
                    int hexB = getc();
                    if (hexA == EOF || hexB == EOF) {
                        THROW(ParseError, "Unexpected end of file while parsing '\\x' escape sequence.", context().loc());
                    }
                    std::string hex = {(char)hexA, (char)hexB};
                    size_t pos;
                    int c_hex = std::stoi(hex, &pos, 16);
                    if (pos != 2) {
                        THROW(ParseError, "Malformed hexidecimal number in '\\x' escape sequence.", context().loc());
                    }
                    result.push_back(c_hex);
                }
            } else {
                result.push_back(c);
            }
        }

        return result;
    }

    int peek(size_t offset = 1) {
        return context().input.peek(offset);
    }

    int getc() {
        return context().input.getc();
    }

    void advance(size_t offset = 1) {
        context().input.advance(offset);
    }

    bool scan_eq_advance(const std::string& value) {
        return context().input.scan_eq_advance(value);
    }

    void skip_whitespace() {
        while (isspace(peek())) {
            advance();
        }
    }
};

//-------------------------------------------------------------------
class ValueState;
class ObjectValueState : public State {
public:
    ObjectValueState(Value::Pointer obj) : _object(obj) { }

    const char* tracer_name() const override {
        return "ObjectValueState";
    }

    void run() override {
        skip_whitespace();

        if (key == "") {
            key = parse_literal();

        } else if (value == nullptr) {
            int c = getc();
            if (c != ':') {
                THROW(ParseError, "Missing colon between object key and value.", context().loc());
            }
            push<ValueState>(&value);

        } else {
            obj().set(key, value);

            int c = peek();

            if (c == ',') {
                advance();
            }

            if (c == ',' || c == '}') {
                pop();
            } else {
                THROW(ParseError, "Missing comma between object values.", context().loc());
            }
        }
    }

    Object& obj() {
        return _object->ref<Object>();
    }

private:
    std::string key = "";
    Value::Pointer value = nullptr;
    Value::Pointer _object;
};

//-------------------------------------------------------------------
class ObjectState : public State {
public:
    ObjectState(Value::Pointer object) : _object(object) { }

    const char* tracer_name() const override {
        return "ObjectState";
    }

    void run() override {
        skip_whitespace();

        int c = peek();

        if (c == '}') {
            advance();
            pop();
            return;
        }

        push<ObjectValueState>(_object);
    }

private:
    Value::Pointer _object;
};

//-------------------------------------------------------------------
class ArrayValueState : public State {
public:
    ArrayValueState(Value::Pointer array) : _array(array) { }

    const char* tracer_name() const override {
        return "ArrayValueState";
    }

    void run() override {
        skip_whitespace();

        if (value == nullptr) {
            push<ValueState>(&value);
            return;
        }

        array().append(value);

        int c = peek();
        if (c == ',') {
            advance();
        }

        if (c == ',' || c == ']') {
            pop();
        } else {
            THROW(ParseError, "Missing comma between array values.", context().loc());
        }
    }

private:
    Array& array() {
        return _array->ref<Array>();
    }

    Value::Pointer value = nullptr;
    Value::Pointer _array;
};

//-------------------------------------------------------------------
class ArrayState : public State {
public:
    ArrayState(Value::Pointer array) : _array(array) { }

    const char* tracer_name() const override {
        return "ArrayState";
    }

    void run() override {
        skip_whitespace();
        int c = peek();

        if (c == ']') {
            advance();
            pop();
            return;
        }

        push<ArrayValueState>(_array);
    }

private:
    Value::Pointer _array;
};

//-------------------------------------------------------------------
class ValueState : public State {
public:
    ValueState(Value::Pointer* value_out) : value_out(value_out) { }

    const char* tracer_name() const override {
        return "ValueState";
    }

    void run() override {
        skip_whitespace();
        int c = peek();

        if (c == '{') {
            (*value_out) = Value::of(Object());
            advance();
            transition<ObjectState>(*value_out);

        } else if (c == '[') {
            (*value_out) = Value::of(Array());
            advance();
            transition<ArrayState>(*value_out);

        } else if (c == '"') {
            (*value_out) = Value::of(parse_literal());
            pop();

        } else if (c == '-' || c == '.' || isdigit(c)) {
            (*value_out) = Value::of(parse_double());
            pop();

        } else if (scan_eq_advance("true")) {
            (*value_out) = Value::of(true);
            pop();

        } else if (scan_eq_advance("false")) {
            (*value_out) = Value::of(false);
            pop();

        } else if (scan_eq_advance("null")) {
            (*value_out) = Value::of(nullptr);
            pop();

        } else {
            THROW(ParseError, "Unexpected character in value expression.", context().loc());
        }
    }

private:
    Value::Pointer* value_out;
};

//-------------------------------------------------------------------
class Parser {
public:
    Parser(std::istream& in, const std::string& filename = "<input>")
    : ctx({
        .input = file::BufferedInput(in, filename)
    }),
    machine(State::Machine::init<ValueState>(ctx, &value)) {
#ifdef MOONLIGHT_JSON_PARSER_DEBUG
        machine.add_tracer([](State::Machine::TraceEvent event,
                              Context& context,
                              const std::string& event_name,
                              const std::vector<State::Pointer>& stack,
                              State::Pointer old_state,
                              State::Pointer new_state) {
            (void) event;
            std::vector<const char*> stack_names = collect::map<const char*>(
                stack, [](auto state) {
                    return state->tracer_name();
                }
            );
            std::string hr(70, '-');
            std::string old_state_name = old_state == nullptr ? "(null)" : old_state->tracer_name();
            std::string new_state_name = new_state == nullptr ? "(null)" : new_state->tracer_name();
            std::cerr << hr << std::endl
                      << "cursor=" << context.dbg_cursor() << std::endl
                      << "stack=" << str::join(stack_names, ",") << std::endl
                      << event_name << ": " << old_state_name << " -> " << new_state_name << std::endl
                      << std::endl;
        });
#endif
    }

    Value::Pointer parse() {
        machine.run_until_complete();
        return value;
    }

private:
    Value::Pointer value = nullptr;
    Context ctx;
    State::Machine machine;
};

}
}
}


#endif /* !__MOONLIGHT_JSON_PARSER_H */
