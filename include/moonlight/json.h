/*
 * json.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday April 6, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_H
#define __MOONLIGHT_JSON_H

#include "moonlight/exceptions.h"
#include "moonlight/automata.h"
#include "moonlight/file.h"
#include "moonlight/meta.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <cctype>

namespace moonlight {
namespace json {

//-------------------------------------------------------------------
class Error : public moonlight::core::Exception {
    using Exception::Exception;
};

//-------------------------------------------------------------------
class ValueError : public Error {
    using Error::Error;
};

//-------------------------------------------------------------------
class KeyNotFoundError : public ValueError {
    using ValueError::ValueError;
};

//-------------------------------------------------------------------
class ArrayIndexOutOfBoundsError : public ValueError {
    using ValueError::ValueError;
};

//-------------------------------------------------------------------
class TypeError : public ValueError {
    using ValueError::ValueError;
};

//-------------------------------------------------------------------
class Value {
public:
    typedef std::shared_ptr<Value> Pointer;

    enum class Type {
        NONE,
        BOOLEAN,
        NUMBER,
        STRING,
        OBJECT,
        ARRAY
    };

    Value(Type type) : _type(type) { }

    Type type() const {
        return _type;
    }

    virtual Value::Pointer clone() const = 0;

    template<class T>
    bool is() const {
        return false;
    }

    template<class T>
    static Value::Pointer of(const T& value) {
        (void) value;
        static_assert(always_false<T>(), "Value of the given type can't be interred.");
    }

    template<>
    Value::Pointer of(const Value::Pointer& value) {
        return value;
    }

    template<class T>
    T get() const {
        static_assert(always_false<T>(), "Value can't be extracted to the given type.");
    }

    template<class T>
    T& ref() {
        static_assert(always_false<T>(), "Value can't be referenced via the given type.");
    }

private:
    Type _type;
};

#define VALUE_IS(Type, Enum) \
template<> \
inline bool Value::is<Type>() const { \
    return type() == Enum; \
}

#define VALUE_OF(Type, Expr) \
template<> \
inline Value::Pointer Value::of(const Type& value) { \
    (void) value; \
    return (Expr).clone(); \
}

#define VALUE_GET(ValueType, ContainerType) \
template<> \
inline ValueType Value::get() const { \
    if (! is<ValueType>()) { \
        throw TypeError("Value is not the expected type."); \
    } \
    return static_cast<const ContainerType*>(this)->value<ValueType>(); \
}

#define VALUE_REF(RefType) \
template<> \
inline RefType& Value::ref() { \
    if (! is<RefType>()) { \
        throw TypeError("Value is not the expected type."); \
    } \
    return static_cast<RefType&>(*this); \
}

VALUE_OF(Value, value);

//-------------------------------------------------------------------
class Null : public Value {
public:
    Null() : Value(Type::NONE) { }

    Value::Pointer clone() const override {
        return std::make_shared<Null>();
    }
};

VALUE_IS(Null, Type::NONE);
VALUE_IS(std::nullptr_t, Type::NONE);
VALUE_OF(std::nullptr_t, Null());

//-------------------------------------------------------------------
class Boolean : public Value {
public:
    Boolean(bool value) : Value(Type::BOOLEAN), _value(value) { }
    Boolean() : Boolean(false) { }

    template<class T>
    T value() const {
        return static_cast<T>(_value);
    }

    template<class T>
    Boolean& set(T value) {
        _value = static_cast<bool>(value);
        return *this;
    }

    Value::Pointer clone() const override {
        return std::make_shared<Boolean>(_value);
    }

private:
    bool _value;
};

VALUE_IS(Boolean, Type::BOOLEAN);
VALUE_OF(bool, Boolean(value));
VALUE_GET(bool, Boolean);

//-------------------------------------------------------------------
class Number : public Value {
public:
    Number(double value) : Value(Type::NUMBER), _value(value) { }
    Number() : Number(0.0) { }

    template<class T>
    T value() const {
        return static_cast<T>(_value);
    }

    template<class T>
    Number& set(T value) {
        _value = static_cast<double>(value);
        return *this;
    }

    Value::Pointer clone() const override {
        return std::make_shared<Number>(_value);
    }

private:
    double _value;
};

VALUE_IS(Number, Type::NUMBER);
VALUE_OF(Number, value);
VALUE_IS(double, Type::NUMBER);
VALUE_OF(double, Number(value));
VALUE_GET(double, Number);
VALUE_IS(float, Type::NUMBER);
VALUE_OF(float, Number(value));
VALUE_GET(float, Number);
VALUE_IS(int, Type::NUMBER);
VALUE_OF(int, Number(value));
VALUE_GET(int, Number);
VALUE_IS(long, Type::NUMBER);
VALUE_OF(long, Number(value));
VALUE_GET(long, Number);

//-------------------------------------------------------------------
class String : public Value {
public:
    String(const std::string& str) : Value(Type::STRING), _str(str) { }
    String(const char* str) : String(std::string(str)) { }
    String() : String("") { }

    template<class T>
    const std::string& value() const {
        static_assert(always_false<T>(), "Value can't be extracted to a string.");
    }

    template<>
    const std::string& value<std::string>() const {
        return _str;
    }

    String& set(const std::string& str) {
        _str = str;
        return *this;
    }

    Value::Pointer clone() const override {
        return std::make_shared<String>(_str);
    }

private:
    std::string _str;
};

VALUE_IS(String, Type::STRING);
VALUE_IS(std::string, Type::STRING);
VALUE_OF(std::string, String(value));
VALUE_GET(std::string, String);

//-------------------------------------------------------------------
class Object : public Value {
public:
    typedef std::unordered_map<std::string, Value::Pointer> Namespace;

    Object(const Namespace& ns) : Value(Type::OBJECT), _ns(ns) { }
    Object(const Object& obj) : Object(obj._ns) { }
    Object() : Value(Type::OBJECT) { }

    bool contains(const std::string& name) const {
        return _ns.find(name) != _ns.end();
    }

    template<class T>
    Object value() const {
        return Object(*this);
    }

    Value::Pointer clone() const override {
        return std::make_shared<Object>(*this);
    }

    template<class T>
    T get(const std::string& name) {
        auto value = _get_value(name);
        if (value == nullptr) {
            throw KeyNotFoundError(name);
        }
        if (! value->is<T>()) {
            throw TypeError(name);
        }
        return static_cast<T>(*value);
    }

    template<>
    Value::Pointer get(const std::string& name) {
        return _get_value(name);
    }

    template<class T>
    T get(const std::string& name, const T& default_value) {
        auto value = _get_value(name);
        if (value == nullptr) {
            return default_value;
        }
        if (! value->is<T>()) {
            throw TypeError(name);
        }
        return value->get<T>();
    }

    template<>
    Value::Pointer get(const std::string& name, const Value::Pointer& default_value) {
        auto value = _get_value(name);
        if (value == nullptr) {
            return default_value;
        }
        return value;
    }

    template<class T>
    Object& set(const std::string& name, const T& value) {
        _ns.insert({name, Value::of(value)});
        return *this;
    }

    template<class T>
    T get_or_set(const std::string& name, const T& default_value) {
        auto value = _get_value(name);
        if (value == nullptr) {
            set(name, default_value);
            value = _get_value(name);
        }
        if (! value->is<T>()) {
            throw TypeError(name);
        }
        return static_cast<T>(*value);
    }

    template<>
    Value::Pointer get_or_set(const std::string& name, const Value::Pointer& default_value) {
        auto value = get<Value::Pointer>(name);
        if (value == nullptr) {
            return set(name, default_value).get<Value::Pointer>(name);
        }
        return value;
    }

    Object& unset(const std::string& name) {
        auto iter = _ns.find(name);
        if (iter != _ns.end()) {
            _ns.erase(iter);
        }
        return *this;
    }

private:
    const Value::Pointer _get_value(const std::string& name) const {
        auto iter = _ns.find(name);
        if (iter == _ns.end()) {
            return nullptr;
        }
        return iter->second;
    }

    Namespace _ns;
};

VALUE_IS(Object, Type::OBJECT);
VALUE_OF(Object, value);
VALUE_GET(Object, Object);
VALUE_REF(Object);

//-------------------------------------------------------------------
class Array : public Value {
public:
    Array() : Value(Type::ARRAY) { }
    Array(const std::vector<Value::Pointer>& vec) : Value(Type::ARRAY), _vec(vec) { }

    template<class T>
    Array(const std::vector<T>& vec) {
        std::transform(
            vec.begin(),
            vec.end(),
            std::back_inserter(_vec),
            [](const auto& v) { return Value::of(v); });
    }

    template<>
    Array(const std::vector<Value::Pointer>& vec) : Value(Type::ARRAY), _vec(vec) { }

    template<class T>
    Array value() const {
        return Array(_vec);
    }

    Value::Pointer clone() const override {
        return std::make_shared<Array>(_vec);
    }

    Array& clear() {
        _vec.clear();
        extract<Array>()[0].extract<int>();
        return *this;
    }

    template<class T>
    std::vector<T> extract() const {
        std::vector<T> vec;
        std::transform(
            _vec.begin(),
            _vec.end(),
            std::back_inserter(vec),
            [](const auto& v) { return v->template get<T>(); });
        return vec;
    }

    template<class T>
    Array& append(const T& value) {
        _vec.push_back(Value::of(value));
        return *this;
    }

    template<class T>
    Array& extend(const std::vector<T>& vec) {
        for (const auto& v : vec) {
            append(v);
        }
        return *this;
    }

    Array& extend(const Array& array) {
        return extend(array._vec);
    }

    template<class T>
    T get(unsigned int offset) const {
        return _cget(offset)->get<T>();
    }

    template<class T>
    Array& set(unsigned int offset, const T& value) {
        _get(offset) = Value::of(value);
    }

    template<class T>
    T pop(std::optional<unsigned int> offset = {}) {
        Value::Pointer value;
        if (offset.has_value()) {
            value = _get(offset.value());
            _vec.erase(_vec.begin() + offset.value());

        } else {
            value = _back();
            _vec.pop_back();
        }

        return value->get<T>();
    }

    unsigned int size() const {
        return _vec.size();
    }

    bool empty() const {
        return size() == 0;
    }

private:
    Value::Pointer _cget(unsigned int offset) const {
        if (offset >= _vec.size()) {
            return nullptr;
        }
        return _vec.at(offset);
    }

    Value::Pointer& _get(unsigned int offset) {
        if (offset >= _vec.size()) {
            throw new ArrayIndexOutOfBoundsError(std::to_string(offset));
        }
        return _vec.at(offset);
    }

    Value::Pointer& _back() {
        if (empty()) {
            throw ArrayIndexOutOfBoundsError("Array is empty.");
        }
        return _vec.back();
    }

    std::vector<Value::Pointer> _vec;
};

VALUE_IS(Array, Type::ARRAY);
VALUE_OF(Array, value);
VALUE_GET(Array, Array);
VALUE_REF(Array);

namespace parser {

//-------------------------------------------------------------------
struct Location {
    std::string name;
    int line;
    int col;

    friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
        out << "Location<" << loc.name
            << ", line " << loc.line
            << ", col " << loc.col
            << ">";
        return out;
    }
};

//-------------------------------------------------------------------
class Error : public moonlight::json::Error {
public:
    Error(const std::string& msg, const Location& loc) :
    moonlight::json::Error(format_message(msg, loc)), _loc(loc) { }

    static std::string format_message(const std::string& msg,
                                      const Location& loc) {
        std::ostringstream sb;
        sb << msg << "(" << loc << ")";
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
};

//-------------------------------------------------------------------
class State : public automata::State<Context> {
protected:
    Error error(const std::string& msg) {
        return Error(msg, context().loc());
    }

    double parse_double() {
        std::string double_str;

        for (;;) {
            int c = peek();
            if (c == '-' || c == '.' || isdigit(c)) {
                double_str.push_back(c);
                advance();
            } else {
                break;
            }
        }

        size_t pos;
        double result = std::stod(double_str, &pos);
        if (pos != double_str.size()) {
            throw error("Malformed double precision value.");
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
            throw error("Input is not a string literal.");
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
                        throw error("Unexpected end of file while parsing '\\x' escape sequence.");
                    }
                    std::string hex = {(char)hexA, (char)hexB};
                    size_t pos;
                    int c_hex = std::stoi(hex, &pos, 16);
                    if (pos != 2) {
                        throw error("Malformed hexidecimal number in '\\x' escape sequence.");
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
    ObjectValueState(Object& obj) : obj(obj) { }

    void run() {
        skip_whitespace();

        if (key == "") {
            key = parse_literal();

        } else if (value == nullptr) {
            int c = getc();
            if (c != ':') {
                throw error("Missing colon between object key and value.");
            }
            push<ValueState>(value);

        } else {
            obj.set(key, value);

            int c = peek();

            if (c == ',') {
                advance();
            }

            if (c == ',' || c == '}') {
                pop();
            } else {
                throw error("Missing comma between object values.");
            }
        }
    }

private:
    std::string key = "";
    Value::Pointer value = nullptr;
    Object& obj;
};

//-------------------------------------------------------------------
class ObjectState : public State {
public:
    ObjectState(Object& obj) : obj(obj) { }

    void run() {
        skip_whitespace();

        int c = peek();

        if (c == '}') {
            pop();
            return;
        }

        push<ObjectValueState>(obj);
    }

private:
    Object& obj;
};

//-------------------------------------------------------------------
class ArrayValueState : public State {
public:
    ArrayValueState(Array& array) : array(array) { }

    void run() {
        skip_whitespace();

        if (value == nullptr) {
            push<ValueState>(value);
            return;
        }

        array.append(value);

        int c = peek();
        if (c == ',') {
            advance();
        }

        if (c == ',' || c == ']') {
            pop();
        } else {
            throw error("Missing comma between array values.");
        }
    }

private:
    Value::Pointer value = nullptr;
    Array& array;
};

//-------------------------------------------------------------------
class ArrayState : public State {
public:
    ArrayState(Array& array) : array(array) { }

    void run() {
        skip_whitespace();
        int c = peek();

        if (c == ']') {
            pop();
            return;
        }

        push<ArrayValueState>(array);
    }

private:
    Array& array;
};

//-------------------------------------------------------------------
class ValueState : public State {
public:
    ValueState(Value::Pointer& value_out) : value_out(value_out) { }

    void run() {
        skip_whitespace();
        int c = peek();

        if (c == '{') {
            value_out = Value::of(Object());
            advance();
            transition<ObjectState>(value_out->ref<Object>());

        } else if (c == '[') {
            value_out = Value::of(Array());
            advance();
            transition<ArrayState>(value_out->ref<Array>());

        } else if (c == '"') {
            value_out = Value::of(parse_literal());
            pop();

        } else if (c == '-' || c == '.' || isdigit(c)) {
            value_out = Value::of(parse_double());
            pop();

        } else if (scan_eq_advance("true")) {
            value_out = Value::of(true);
            pop();

        } else if (scan_eq_advance("false")) {
            value_out = Value::of(false);
            pop();

        } else if (scan_eq_advance("null")) {
            value_out = Value::of(nullptr);
            pop();

        } else {
            throw error("Unexpected character in value expression.");
        }
    }

private:
    Value::Pointer& value_out;
};

//-------------------------------------------------------------------
class Parser {
public:
    Parser(std::istream& in, const std::string& filename = "<input>")
    : ctx({
        .input = file::BufferedInput(in, filename)
    }),
    machine(State::Machine::init<ValueState>(ctx, value)) { }

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

namespace serialize {

}

}
}


#endif /* !__MOONLIGHT_JSON_H */
