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

#include "moonlight/json/parser.h"
#include "moonlight/json/serializer.h"
#include "moonlight/json/mapping.h"

namespace moonlight {
namespace json {

//-------------------------------------------------------------------
struct Indent {
    bool pretty = true;
    int indent = 4;
};

//-------------------------------------------------------------------
inline Value::Pointer _parse(std::istream& input, const std::string& filename = "<input>") {
    parser::Parser parser(input, filename);
    return parser.parse();
}

//-------------------------------------------------------------------
template<class T>
T _adt_from_json(const Value& json) {
    static_assert(has_dunder_json<T>() ||
                  is_map_type<T>() ||
                  is_iterable_type<T>(),
                  "Value can't be extracted to the given type.");

    if constexpr (has_dunder_json<T>()) {
        // json must be an object.
        if (json.type() != Value::Type::OBJECT) {
            throw TypeError("Can't map non-object value into class object.");
        }

        return T().__json__().map_from_json(static_cast<const Object&>(json));

    } else if constexpr (is_map_type<T>()) {
        // json must be an object.
        if (json.type() != Value::Type::OBJECT) {
            throw TypeError("Can't map non-object value into map.");
        }

        const Object& obj = static_cast<const Object&>(json);
        return obj.extract<T::mapped_type>();

    } else /* if is_iterable_type<T>() */ {
        // json must be an array.
        if (json.type() != Value::Type::ARRAY) {
            throw TypeError("Can't map non-array value into iterable sequence.");
        }
        T iterable;
        const Array& array = static_cast<const Array&>(json);
        return array.extract<T::value_type>();
    }
}

//-------------------------------------------------------------------
template<class T>
Value::Pointer _adt_to_json(const T& obj) {
    static_assert(has_dunder_json<T>() ||
                  is_map_type<T>() ||
                  is_iterable_type<T>(),
                  "Value can't be converted to json.");

    if constexpr (has_dunder_json<T>()) {
        return const_cast<T&>(obj).__json__().map_to_json().clone();

    } else if constexpr (is_map_type<T>()) {
        std::shared_ptr<Object> json_obj = std::make_shared<Object>();
        for (auto iter = obj.begin(); iter != obj.end(); iter++) {
            json_obj->set(iter->first, iter->second);
        }
        return json_obj;

    } else /* if (is_iterable_type<T>() */ {
        std::shared_ptr<Array> array = std::make_shared<Array>();

        for (auto item : obj) {
            array->append(item);
        }

        return array;
    }
}

//-------------------------------------------------------------------
template<class T>
T read(std::istream& input, const std::string& filename = "<input>") {
    auto value = _parse(input, filename);
    return value->get<T>();
}

//-------------------------------------------------------------------
template<>
inline Value::Pointer read(std::istream& input, const std::string& filename) {
    return _parse(input, filename);
}

//-------------------------------------------------------------------
template<class T>
T read(const std::string& json_str) {
    std::istringstream infile(json_str);
    return read<T>(infile, "<str>");
}

//-------------------------------------------------------------------
template<class T>
T read_file(const std::string& filename) {
    auto infile = file::open_r(filename);
    return read<T>(infile, filename);
}

//-------------------------------------------------------------------
inline void write(std::ostream& out, const Value& value, Indent idt = Indent()) {
    serializer::Serializer s(out);
    s.pretty(idt.pretty);
    s.indent_width(idt.indent);
    s.serialize(value);
}

//-------------------------------------------------------------------
inline void write(std::ostream& out, Value::Pointer value, Indent idt = Indent()) {
    write(out, value->ref<Value>(), idt);
}

//-------------------------------------------------------------------
inline void write_file(const std::string& filename, const Value& value, Indent idt = Indent()) {
    auto outfile = file::open_w(filename);
    write(outfile, value, idt);
}

//-------------------------------------------------------------------
inline void write_file(const std::string& filename, Value::Pointer value, Indent idt = Indent()) {
    write_file(filename, value->ref<Value>(), idt);
}

//-------------------------------------------------------------------
inline std::string to_string(const Value& value, Indent idt = Indent()) {
    std::ostringstream sb;
    write(sb, value, idt);
    return sb.str();
}

//-------------------------------------------------------------------
inline std::string to_string(Value::Pointer value, Indent idt = Indent()) {
    return to_string(value->ref<Value>(), idt);
}

//-------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& out, const Value& value) {
    write(out, value, {.pretty=false});
    return out;
}

//-------------------------------------------------------------------
inline std::ostream& operator<<(std::ostream& out, Value::Pointer value) {
    write(out, *value, {.pretty=false});
    return out;
}

//-------------------------------------------------------------------
template<class T>
json::Object map(const T& obj) {
    return const_cast<T&>(obj).__json__().map_to_json();
}

//-------------------------------------------------------------------
template<class T>
T map(const Object& json_obj) {
    return T().__json__().map_from_json(json_obj);
}

}
}

#endif /* !__MOONLIGHT_JSON_H */
