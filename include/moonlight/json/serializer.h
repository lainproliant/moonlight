/*
 * serializer.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_SERIALIZER_H
#define __MOONLIGHT_JSON_SERIALIZER_H

#include "moonlight/json/array.h"
#include "moonlight/json/object.h"
#include "moonlight/collect.h"
#include <sstream>
#include <cassert>

namespace moonlight {
namespace json {
namespace serializer {

class Serializer {
public:
    Serializer(std::ostream& out) : _out(out) { }

    void serialize(const Value& value, unsigned int ind = 0) {
        switch(value.type()) {
        case Value::Type::ARRAY:
            serialize_array(value.cref<Array>(), ind);
            break;
        case Value::Type::BOOLEAN:
            serialize_boolean(value.cref<Boolean>());
            break;
        case Value::Type::NUMBER:
            serialize_number(value.cref<Number>());
            break;
        case Value::Type::OBJECT:
            serialize_object(value.cref<Object>(), ind);
            break;
        case Value::Type::STRING:
            serialize_string(value.cref<String>());
            break;
        case Value::Type::NONE:
            serialize_null(value.cref<Null>());
            break;
        }
    }

    bool pretty() const { return _pretty; }

    Serializer& pretty(bool value) {
        _pretty = value;
        return *this;
    }

    Serializer& sort_keys(bool value) {
        _sort_keys = value;
        return *this;
    }

    int indent_width() const {
        return _indent_width;
    }

    Serializer& indent_width(int width) {
        _indent_width = width;
        return *this;
    }

private:
    void serialize_array(const Array& array, unsigned int ind) {
        _out << "[";
        if (array.empty()) {
            _out << "]";
            return;
        }
        for (unsigned int x = 0; x < array.size(); x++) {
            if (pretty()) _out << "\n";
            indent(ind + 1);
            serialize(array.get<Value::Pointer>(x)->ref<Value>(), ind + 1);
            if (x + 1 < array.size()) {
                _out << ',';
            }
        }
        if (pretty()) _out << "\n";
        indent(ind);
        _out << "]";
    }

    void serialize_boolean(const Boolean& boolean) {
        _out << (boolean.get<bool>() ? "true" : "false");
    }

    void serialize_number(const Number& number) {
        _out << number.get<double>();
    }

    void serialize_object(const Object& obj, unsigned int ind) {
        _out << "{";
        if (obj.empty()) {
            _out << "}";
            return;
        }

        auto keys = obj.keys();
        if (_sort_keys) {
            keys = collect::sorted(keys);
        }

        for (unsigned int x = 0; x < keys.size(); x++) {
            auto val = obj.get<Value::Pointer>(keys[x]);
            if (pretty()) _out << "\n";
            indent(ind + 1);
            _out << "\"" << str::literal(keys[x]) << "\":";
            if (pretty()) _out << ' ';
            serialize(val->ref<Value>(), ind + 1);
            if (x + 1 < keys.size()) {
                _out << ',';
            }
        }
        if (pretty()) _out << "\n";
        indent(ind);
        _out << "}";
    }

    void serialize_string(const String& value) {
        _out << "\"" << str::literal(value.get<std::string>()) << "\"";
    }

    void serialize_null(const Null& null) {
        (void) null;
        _out << "null";
    }

    void indent(unsigned int ind) {
        for (unsigned int x = 0; x < (ind * indent_width()) && pretty(); x++) {
            _out << ' ';
        }
    }

    bool _pretty = true;
    bool _sort_keys = false;
    unsigned int _indent_width = 4;
    std::ostream& _out;
};


}
}
}


#endif /* !__MOONLIGHT_JSON_SERIALIZER_H */
