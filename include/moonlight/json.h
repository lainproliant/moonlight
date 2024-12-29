/*
 * ## json.h --------------------------------------------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday April 6, 2021
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This is a fairly comprehensive hand-written JSON parsing library which aims
 * to simplify C++ data marshalling and unmarshalling.  It is inspired by
 * `picojson` by Kazuho Oku.
 *
 * This library offers the following high-level JSON data structures:
 *
 * - `json::Value`: The abstract base to all other JSON data structures.
 *   `Value::Pointer` is a smart pointer type for containing pointers to
 *   variant JSON data structures, where `type()` can be called to determine
 *   which `Value::Type` the data structure is, then converted to the appropriate
 *   type via `std::dynamic_pointer_cast<T>(p)`.
 * - `json::Null`: Represents the JSON `null` singleton and type.  All
 *   `json::Null` objects are equivalent.
 * - `json::Boolean`: Represents the JSON `bool` type, which can be `true` or `false`.
 * - `json::Number`: Represents the JSON `number` type, which is a double
 *   precision floating point number.
 * - `json::String`: Represents the JSON `string` type.
 * - `json::Array`: Represents the JSON `array` type, which is a heterogenous
 *   linear collection of objects.  The `extract<T>()` method can be used to
 *   extract a homogenous `std::vector<T>` from this array structure, which
 *   will throw a `core::TypeError` if any of the array elements can't be
 *   converted to type `T`.
 * - `json::Object`: Represents the JSON `object` type, which is a key value
 *   map with string keys and arbitrary values.  The `extract<T,
 *   M=linked_map<std::string, T>>()` method can be used to extract a
 *   homogenous map from this object structure, which will throw a
 *   `core::TypeError` if any of the map values can't be converted to type `T`.
 *   Note that `json::Object` is also aliased to `json::JSON`.
 *
 * This library also offers the following template functions for marshalling
 * and unmarshalling JSON data:
 *
 * - `Value::of(v)`: Interprets any JSON-mappable data structure as a `Value`,
 *   returning a `Value::Pointer` to the newly created JSON data structure.
 * - `read<T>(out, filename="<input>")`: Reads a data structure of type `T`
 *   from the given JSON input stream.  `filename` controls the name of the
 *   file, which is referenced in JSON parsing diagnostic output such as
 *   exception messages.
 * - `read<T>(s)`: Reads an object of type `T` from the JSON string `s`.
 * - `read_file<T>(name)`: Opens a JSON file and reads an object of type `T`.
 * - `write(out, v, idt=FormatOptions())`: Writes an object `v` as JSON to the
 *   output stream `out`, using the given indent settings if provided.
 * - `write_file(name, v, idt=FormatOptions())`: Writes an object `v` as JSON
 *   to the given JSON file, using the given indent settings if provided.
 * - `to_string(v, idt=FormatOptions())`: Writes an object `v` as JSON to an
 *   `std::string`, using the given indent settings if provided.
 * - `map(v)`: Converts a non-JSON object to a JSON object, or vise versa.
 *
 * To further assist in JSON marhsalling and unmarshalling, this library
 * includes `json/mapping.h`, a high level paradigm for automatically mapping
 * C++ class data to and from JSON data structures.  In addition to iterable
 * collections, maps, strings, numbers, booleans, and null types, any class
 * type which has defined a publically accessible `__json__()` method returning
 * a `json::Mapper<T>` is considered to be JSON-mappable.  Here's an example of
 * how to define a class which is JSON-mappable:
 *
 * ```
 * class Name {
 * public:
 *    std::string get_first() const {
 *       return first;
 *    }
 *
 *    void set_first(const std::string& v) {
 *       first = v;
 *    }
 *
 *    std::string get_last() const {
 *       return last;
 *    }
 *
 *    void set_last(const std::string& v) {
 *       last = v;
 *    }
 *
 *    json::Mapper<Name> __json__() {
 *       return json::Mapper(this)
 *          .field("first", first)
 *          .property("last", &Person::get_last, &Person::set_last);
 *    }
 *
 * private:
 *    std::string first;
 *    std::string last;
 * };
 * ```
 *
 * For illustrative purposes, the `Name` class above uses both `field()` and
 * `property()` mapping methods to define JSON mappings.  The `field()` method
 * maps values to and from JSON by referencing the class field directly,
 * whereas the `property()` method uses a "getter" and "setter" method to map
 * to and from JSON.
 */

#ifndef __MOONLIGHT_JSON_H
#define __MOONLIGHT_JSON_H

#include <memory>
#include <string>

#include "moonlight/json/parser.h"
#include "moonlight/json/serializer.h"

namespace moonlight {
namespace json {

inline Value::Pointer _parse(std::istream& input, const std::string& filename = "<input>") {
    parser::Parser parser(input, filename);
    return parser.parse();
}

template<class T>
void _adt_from_json_impl(T& adt, const Value& json) {
    static_assert(has_dunder_json<T>() ||
                  is_map_type<T>() ||
                  is_iterable_type<T>(),
                  "Value can't be extracted to the given type.");

    if constexpr (has_dunder_json<T>()) {
        // json must be an object.
        if (json.type() != Value::Type::OBJECT) {
            THROW(core::TypeError, "Can't map non-object value into class object.");
        }

        adt = adt.__json__().map_from_json(static_cast<const Object&>(json));

    } else if constexpr (is_map_type<T>()) {
        // json must be an object.
        if (json.type() != Value::Type::OBJECT) {
            THROW(core::TypeError, "Can't map non-object value into map.");
        }

        const Object& obj = static_cast<const Object&>(json);
        adt.clear();
        for (auto pair : obj.extract<typename T::mapped_type>()) {
            adt.insert(pair);
        }

    } else /* if is_iterable_type<T>() */ {
        // json must be an array.
        if (json.type() != Value::Type::ARRAY) {
            THROW(core::TypeError, "Can't map non-array value into iterable sequence.");
        }

        const Array& array = static_cast<const Array&>(json);
        adt.clear();
        for (auto value : array.extract<typename T::value_type>()) {
            adt.push_back(value);
        }
    }
}

template<class T>
T _adt_from_json(const Value& json) {
    static_assert(has_dunder_json<T>() ||
                  is_map_type<T>() ||
                  is_iterable_type<T>(),
                  "Value can't be extracted to the given type.");

    T adt;
    _adt_from_json_impl(adt, json);
    return adt;
}

template<class T>
Value::Pointer _adt_to_json_impl(const T& adt) {
    static_assert(has_dunder_json<T>() ||
                  is_map_type<T>() ||
                  is_iterable_type<T>(),
                  "Value can't be converted to json.");

    if constexpr (has_dunder_json<T>()) {
        return const_cast<T&>(adt).__json__().map_to_json().clone();

    } else if constexpr (is_map_type<T>()) {
        std::shared_ptr<Object> json_adt = std::make_shared<Object>();
        for (auto iter = adt.begin(); iter != adt.end(); iter++) {
            json_adt->set(iter->first, iter->second);
        }
        return json_adt;

    } else /* if (is_iterable_type<T>() */ {
        std::shared_ptr<Array> array = std::make_shared<Array>();

        for (auto item : adt) {
            array->append(item);
        }

        return array;
    }
}

template<class T>
Value::Pointer _adt_to_json(const T& adt) {
    return _adt_to_json_impl(adt);
}

template<class T>
T read(std::istream& input, const std::string& filename = "<input>") {
    auto value = _parse(input, filename);
    return value->get<T>();
}

template<>
inline Value::Pointer read(std::istream& input, const std::string& filename) {
    return _parse(input, filename);
}

template<class T>
T read(const std::string& json_str) {
    std::istringstream infile(json_str);
    return read<T>(infile, "<str>");
}

template<class T>
T read_file(const std::string& filename) {
    auto infile = file::open_r(filename);
    return read<T>(infile, filename);
}

template<class T>
void write(std::ostream& out, const T& value, FormatOptions idt = FormatOptions()) {
    write(out, Value::of(value), idt);
}

template<>
inline void write(std::ostream& out, const Value& value, FormatOptions idt) {
    auto s = serializer::Serializer(out).options(idt);
    s.serialize(value);
}

template<>
inline void write(std::ostream& out, const Value::Pointer& value, FormatOptions idt) {
    write(out, value->ref<Value>(), idt);
}

template<class T>
inline void write_file(const std::string& filename, const T& value, FormatOptions idt = FormatOptions()) {
    auto outfile = file::open_w(filename);
    write(outfile, value, idt);
}

template<class T>
std::string to_string(const T& value, FormatOptions idt = FormatOptions()) {
    std::ostringstream sb;
    write(sb, value, idt);
    return sb.str();
}

inline std::ostream& operator<<(std::ostream& out, const Value& value) {
    write(out, value, {.pretty=false});
    return out;
}

inline std::ostream& operator<<(std::ostream& out, Value::Pointer value) {
    write(out, *value, {.pretty=false});
    return out;
}

template<class T>
json::Object map(const T& obj) {
    return const_cast<T&>(obj).__json__().map_to_json();
}

template<class T>
T map(const Object& json_obj) {
    return T().__json__().map_from_json(json_obj);
}

typedef Object JSON;
using JSONArray = Array;

}  // namespace json
}  // namespace moonlight

#endif /* !__MOONLIGHT_JSON_H */
