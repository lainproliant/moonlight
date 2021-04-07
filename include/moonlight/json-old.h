/*
 * moonlight/json.h: JSON wrapper class for simplified object serialization.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Wednesday, Jan 14 2015,
 *       Thursday, Dec 21 2017
 */
#ifndef __MOONLIGHT_JSON_H
#define __MOONLIGHT_JSON_H

#include "moonlight/exceptions.h"
#include "moonlight/file.h"
#include "moonlight/maps.h"
#include "picojson/picojson.h"

#include <memory>
#include <sstream>
#include <optional>

namespace moonlight {
namespace json {

//-------------------------------------------------------------------
class Exception : public core::Exception {
public:
    using core::Exception::Exception;
};

/**------------------------------------------------------------------
 * A wrapper class around picojson with a simplified, object-focused interface.
 *
 * NOTE: This class does not offer direct access to the following structures:
 *    - Documents which are not objects, i.e. list documents.
 *    - Heterogenous lists, i.e. lists must contain only objects, ints, floats,
 *      bools, or strings, and may not mix these data types.
 *    - Lists of lists
 *
 * These limitations are by design and allow for a greatly simplified interface
 * that's useful for settings and object serialization.
 *
 * If such objects need to be accessed, please use picojson directly.
 * You can also load objects that contain forbidden structures, then use
 * Object::get to fetch the wrapped picojson::value and inspect further there.
 */
class Object {
public:
    Object() : obj_value(std::make_shared<picojson::value>(picojson::object())) { }
    Object(const std::shared_ptr<picojson::value>& obj_value) :
    obj_value(obj_value) { }

    virtual ~Object() { }

    static Object load_from_file(const std::string& filename) {
        try {
            std::ifstream infile = file::open_r(filename);
            return load_from_stream(infile);
        } catch (const Exception& e) {
            throw Exception(str::cat(
                    "File does not contain a JSON object: '", filename, "'."
            ));
        }
    }

    static Object load_from_string(const std::string& json_string) {
        std::istringstream sb(json_string);

        try {
            return load_from_stream(sb);
        } catch (const Exception& e) {
            throw Exception("JSON string does not contain an object.");
        }
    }

    static Object load_from_stream(std::istream& infile) {
        std::shared_ptr<picojson::value> obj_value = std::make_shared<picojson::value>();
        infile >> (*obj_value);
        if (! obj_value->is<picojson::object>()) {
            throw Exception("Stream did not contain a JSON object.");
        }
        return Object(obj_value);
    }

    void save_to_file(const std::string& filename, bool prettify = false) const {
        std::ofstream outfile = file::open_w(filename);
        print(outfile, prettify);
    }

    void print(std::ostream& outfile, bool prettify = false) const {
        outfile << to_string(prettify);
    }

    std::string to_string(bool prettify = false) const {
        return obj_value->serialize(prettify);
    }

    bool contains(const std::string& name) const {
        return obj_value->contains(name);
    }

    std::vector<std::string> get_keys() const {
        return maps::keys(obj_value->get<picojson::object>());
    }

    picojson::value& get() {
        return *obj_value;
    }

    const picojson::value& get() const {
        return *obj_value;
    }

    template<class T>
    T get(const std::string& name, std::optional<T> default_value = {}) const {
        if (! contains(name)) {
            if (default_value.has_value()) {
                return default_value.value();
            }
            throw Exception(str::cat("Missing value for key '", name, "'."));
        }
        return _get<T>(name);
    }

    template<class T>
    T set(const std::string& name, const T& value) {
        return _set<T>(name, value);
    }

    template<class T>
    T get_or_set(const std::string& name, const T& default_value) {
        T value = get<T>(name, default_value);
        if (! contains(name)) {
            set<T>(name, value);
        }
        return value;
    }

    template<class T>
    std::vector<T> get_array(const std::string& name) const {
        if (! contains(name)) {
            throw Exception(str::cat(
                    "Missing array for key '", name, "'."
            ));
        }

        return _get_array<T>(name);
    }

    template<class T>
    std::vector<T> get_array(const std::string& name, const std::vector<T>& default_value) const {
        if (! contains(name)) {
            return default_value;
        }

        return _get_array<T>(name);
    }

    template<class T>
    std::vector<T> set_array(const std::string& name, const std::vector<T>& vec) {
        return _set_array(name, vec);
    }

    template<>
    std::vector<Object> set_array(const std::string& name, const std::vector<Object>& vec) {
        picojson::array array;
        picojson::object& obj = obj_value->get<picojson::object>();

        for (Object obj : vec) {
            array.push_back(*(obj.obj_value.get()));
        }

        obj[name] = picojson::value(array);
        return get_array<Object>(name);
    }


    template<class T>
    std::vector<T> get_or_set_array(const std::string& name, const std::vector<T>& default_vec) {
        std::vector<T> vec = get_array<T>(name, default_vec);
        if (! contains(name)) {
            set_array<T>(name, vec);
        }
        return vec;
    }

    friend std::ostream& operator<<(std::ostream& out, const Object& settings) {
        out << settings.to_string();
        return out;
    }

private:
    template<class T>
    T _get(const std::string& name) const {
        picojson::value value = obj_value->get(name);
        if (! value.is<T>()) {
            throw Exception(str::cat("Unexpected value type for key '", name, "'."));
        }

        return value.get<T>();
    }

    template<>
    Object _get(const std::string& name) const {
        picojson::value object_json = obj_value->get(name);
        if (! object_json.is<picojson::object>()) {
            throw Exception(str::cat(
                    "Key '", name, "' does not refer to an object."
            ));
        }

        return Object(std::make_shared<picojson::value>(object_json));
    }

#define DEF_NUMERIC_GET(T) \
    template<> T _get(const std::string& name) const { \
        if (! contains(name)) { \
            throw Exception(str::cat("Missing value for key '", name, "'.")); \
        } \
        return static_cast<T>(_get<double>(name)); \
    }

    template<class T>
    T _set(const std::string& name, const T& value) {
        obj_value->get<picojson::object>()[name] = picojson::value(value);
        return get<T>(name);
    }

#define DEF_NUMERIC_SET(T) \
    template<> \
    T _set(const std::string& name, const T& value) { \
        obj_value->get<picojson::object>()[name] = picojson::value(static_cast<double>(value)); \
        return get<T>(name); \
    }

    template<>
    Object _set(const std::string& name, const Object& value) {
        obj_value->get<picojson::object>()[name] = (*value.obj_value);
        return get<Object>(name);
    }


    template<class T>
    std::vector<T> _get_array(const std::string& name) const {
        std::vector<T> vec;
        const picojson::value& array_value = obj_value->get(name);
        if (! array_value.is<picojson::array>()) {
            throw Exception(str::cat(
                    "Key '", name, "' does not refer to an array."
            ));
        }

        const picojson::array& array = array_value.get<picojson::array>();
        for (auto val : array) {
            if (! val.is<T>()) {
                throw Exception(str::cat(
                        "Unexpected heterogenous value type in array for key '", name, "'."
                ));
            }
            vec.push_back(val.get<T>());
        }

        return vec;
    }

#define DEF_NUMERIC_GET_ARRAY(T) \
    template<> std::vector<T> _get_array(const std::string& name) const { \
        if (! contains(name)) { \
            throw Exception(str::cat( \
                    "Key '", name, "' does not refer to a numeric array." \
            )); \
        } \
        std::vector<T> result; \
        std::vector<double> doubles = _get_array<double>(name); \
        std::transform(doubles.begin(), doubles.end(), std::back_inserter(result), \
                       [](auto dbl) { return static_cast<T>(dbl); }); \
        return result; \
    }

    template<>
    std::vector<Object> _get_array(const std::string& name) const {
        picojson::value obj_values_array_value = obj_value->get(name);
        if (! obj_values_array_value.is<picojson::array>()) {
            throw Exception(str::cat(
                    "Key '", name, "' does not refer to an object array."
            ));
        }

        picojson::array& obj_values_array = obj_values_array_value.get<picojson::array>();
        std::vector<Object> obj_array;

        for (picojson::value val : obj_values_array) {
            if (! val.is<picojson::object>()) {
                throw Exception(str::cat(
                        "JSON array contains non-object: '", name, "'."
                ));
            }

            obj_array.push_back(Object(std::make_shared<picojson::value>(val)));
        }

        return obj_array;
    }

    template<class T>
    std::vector<T> _set_array(const std::string& name, const std::vector<T>& vec) {
        picojson::object& obj = obj_value->get<picojson::object>();
        picojson::array array;
        for (T val : vec) {
            array.push_back(picojson::value(val));
        }
        obj[name] = picojson::value(array);
        return get_array<T>(name);
    }

#define DEF_NUMERIC_SET_ARRAY(T) \
    template<> \
    std::vector<T> _set_array(const std::string& name, const std::vector<T>& vec) { \
        picojson::object& obj = obj_value->get<picojson::object>(); \
        picojson::array array; \
        for (T val : vec) { \
            array.push_back(picojson::value(static_cast<double>(val))); \
        } \
        obj[name] = picojson::value(array); \
        return get_array<T>(name); \
    }

#define DEF_NUMERIC(T) \
    DEF_NUMERIC_GET(T); \
    DEF_NUMERIC_SET(T); \
    DEF_NUMERIC_GET_ARRAY(T); \
    DEF_NUMERIC_SET_ARRAY(T);

    DEF_NUMERIC(int);
    DEF_NUMERIC(float);
    DEF_NUMERIC(long);

    std::shared_ptr<picojson::value> obj_value;
};

//-------------------------------------------------------------------
inline Object load_from_file(const std::string& filename) {
    return Object::load_from_file(filename);
}

//-------------------------------------------------------------------
inline Object load_from_string(const std::string& json_string) {
    return Object::load_from_string(json_string);
}

//-------------------------------------------------------------------
inline Object load_from_stream(std::istream& infile) {
    return Object::load_from_stream(infile);
}

// For backwards compatibility
typedef Object Wrapper;

}
}

#endif /*__MOONLIGHT_JSON_H */
