/*
 * core.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_CORE_H
#define __MOONLIGHT_JSON_CORE_H

#include "moonlight/exceptions.h"
#include "moonlight/traits.h"
#include <memory>
#include <optional>

namespace moonlight {
namespace json {

//-------------------------------------------------------------------
// Template pre-declarations.
//
class Value;

template<class T>
T _adt_from_json(const Value& value);

template<class T>
std::shared_ptr<Value> _adt_to_json(const T& obj);

//-------------------------------------------------------------------
// SFINAE test to determine if type has `__json__` member.
//
template<class T, class = void>
struct has_dunder_json : public std::false_type { };

template<class T>
struct has_dunder_json<T, std::void_t<
decltype(&T::__json__)>> : public std::true_type { };

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
    virtual ~Value() { }

    Type type() const {
        return _type;
    }

    const std::string& type_name() const {
        static const std::map<Type, std::string> TYPE_NAMES = {
            {Type::NONE, "NONE"},
            {Type::BOOLEAN, "BOOLEAN"},
            {Type::NUMBER, "NUMBER"},
            {Type::STRING, "STRING"},
            {Type::OBJECT, "OBJECT"},
            {Type::ARRAY, "ARRAY"}
        };

        return TYPE_NAMES.find(type())->second;
    }

    virtual Value::Pointer clone() const = 0;

    template<class T>
    bool is() const {
        if (has_dunder_json<T>() || is_map_type<T>()) {
            return type() == Type::OBJECT;
        } else if (is_iterable_type<T>()) {
            return type() == Type::ARRAY;
        }
        return false;
    }

    template<class T>
    static Value::Pointer of(const T& value) {
        return _adt_to_json(value);
    }

    Value::Pointer of(const char* value);

    template<class T>
    T get() const {
        return _adt_from_json<T>(*this);
    }

    template<class T>
    T& ref() {
        static_assert(always_false<T>(), "Value can't be referenced via the given type.");
    }

    template<class T>
    const T& cref() const {
        static_assert(always_false<T>(), "Value can't be referenced via the given type.");
    }

private:
    Type _type;
};

template<>
inline bool Value::is<Value>() const {
    return true;
}

template<>
inline Value::Pointer Value::of(const Value::Pointer& value) {
    return value;
}

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
        throw core::TypeError("Value is not the expected type."); \
    } \
    return static_cast<const ContainerType*>(this)->value<ValueType>(); \
}

#define VALUE_REF(RefType) \
template<> \
inline RefType& Value::ref() { \
    if (! is<RefType>()) { \
        throw core::TypeError("Value is not the expected type."); \
    } \
    return static_cast<RefType&>(*this); \
} \
template<> \
inline const RefType& Value::cref() const { \
    if (! is<RefType>()) { \
        throw core::TypeError("Value is not the expected type."); \
    } \
    return static_cast<const RefType&>(*this); \
}

VALUE_OF(Value, value);
VALUE_REF(Value);

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
VALUE_REF(Null);

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
VALUE_IS(bool, Type::BOOLEAN);
VALUE_OF(bool, Boolean(value));
VALUE_GET(bool, Boolean);
VALUE_REF(Boolean);

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
VALUE_REF(Number);

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

template<>
inline const std::string& String::value<std::string>() const {
    return _str;
}

VALUE_IS(String, Type::STRING);
VALUE_IS(std::string, Type::STRING);
VALUE_IS(const std::string, Type::STRING);
VALUE_OF(std::string, String(value));
VALUE_GET(std::string, String);
VALUE_REF(String);

inline Value::Pointer Value::of(const char* value) {
    return String(value).clone();
}

}
}



#endif /* !__MOONLIGHT_JSON_CORE_H */
