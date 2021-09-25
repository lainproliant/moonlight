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

struct Indent {
    bool pretty = true;
    int indent = 4;
};

inline Value::Pointer _parse(std::istream& input, const std::string& filename = "<input>") {
    parser::Parser parser(input, filename);
    return parser.parse();
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

inline void write(std::ostream& out, const Value& value, Indent idt = Indent()) {
    serializer::Serializer s(out);
    s.pretty(idt.pretty);
    s.indent_width(idt.indent);
    s.serialize(value);
}

inline void write(std::ostream& out, Value::Pointer value, Indent idt = Indent()) {
    write(out, value->ref<Value>(), idt);
}

inline void write_file(const std::string& filename, const Value& value, Indent idt = Indent()) {
    auto outfile = file::open_w(filename);
    write(outfile, value, idt);
}

inline void write_file(const std::string& filename, Value::Pointer value, Indent idt = Indent()) {
    write_file(filename, value->ref<Value>(), idt);
}

inline std::string to_string(const Value& value, Indent idt = Indent()) {
    std::ostringstream sb;
    write(sb, value, idt);
    return sb.str();
}

inline std::string to_string(Value::Pointer value, Indent idt = Indent()) {
    return to_string(value->ref<Value>(), idt);
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


}
}

#endif /* !__MOONLIGHT_JSON_H */
