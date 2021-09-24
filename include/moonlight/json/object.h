/*
 * object.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_OBJECT_H
#define __MOONLIGHT_JSON_OBJECT_H

#include "moonlight/json/core.h"
#include "moonlight/maps.h"
#include "moonlight/generator.h"
#include "moonlight/traits.h"

#include <iostream>

namespace moonlight {
namespace json {

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

    Object& operator=(const Object& other) {
        _ns = other._ns;
        return *this;
    }

    template<class T>
    Object value() const {
        return Object(*this);
    }

    Value::Pointer clone() const override {
        return std::make_shared<Object>(*this);
    }

    Object& clear() {
        _ns.clear();
        return *this;
    }

    template<class T, class M = std::unordered_map<std::string, T>>
    M extract() const {
        M map;
        std::transform(_ns.begin(), _ns.end(), std::inserter(map, map.end()),
                       [](const auto& pair) -> std::pair<std::string, T> {
                           return { pair.first, pair.second->template get<T>() };
                       });
        return map;
    }

    template<class T>
    T get(const std::string& name) const {
        auto value = _get_value(name);
        if (value == nullptr) {
            throw KeyNotFoundError(name);
        }
        if (! value->is<T>()) {
            throw TypeError(name);
        }
        return value->get<T>();
    }

    template<>
    Value::Pointer get<Value::Pointer>(const std::string& name) const {
        return _get_value(name);
    }

    template<class T>
    T get(const std::string& name, const T& default_value) const {
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
    Value::Pointer get<Value::Pointer>(const std::string& name, const Value::Pointer& default_value) const {
        auto value = _get_value(name);
        if (value == nullptr) {
            return default_value;
        }
        return value;
    }

    template<class T>
    Object& set(const std::string& name, const T& value) {
        auto iter = _ns.find(name);
        if (iter != _ns.end()) {
            _ns.erase(iter);
        }
        _ns.insert({name, Value::of(value)});
        return *this;
    }

    template<>
    Object& set(const std::string& name, const Value::Pointer& value) {
        _ns.insert({name, value});
        return *this;
    }

    template<class T>
    T get_or_set(const std::string& name, const T& default_value) {
        auto value = _get_value(name);
        if (value == nullptr) {
            set<T>(name, default_value);
            value = _get_value(name);
        }
        if (! value->is<T>()) {
            throw TypeError(name);
        }
        return value->get<T>();
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

    template<class T>
    gen::Iterator<std::pair<std::string, T>> begin() const {
        auto iter = _ns.begin();

        return gen::begin([iter, this]() mutable -> std::optional<T> {
            if (iter == this->_ns.end()) {
                return {};

            } else if constexpr (is_raw_pointer<T>()) {
                return {(iter++)->first, &iter->second->ref<T>()};

            } else {
                return {(iter++)->first, iter->second->get<T>()};
            }
        });
    }

    template<class T>
    gen::Iterator<std::pair<std::string, T>> end() const {
        return gen::end<std::pair<std::string, T>>();
    }

    template<class T>
    gen::Iterable<std::pair<std::string, T>> iterate() const {
        return gen::Iterable<std::pair<std::string, T>>(begin<T>());
    }

    gen::Iterable<std::string> iterate_keys() const {
        auto iter = _ns.begin();

        return gen::Iterable<std::string>(
            gen::begin<std::string>([iter, this]() mutable -> std::optional<std::string> {
                if (iter == this->_ns.end()) {
                    return {};
                } else {
                    return (iter++)->first;
                }
            }
        ));
    }

    template<class T>
    gen::Iterable<T> iterate_values() const {
        auto iter = _ns.begin();

        return gen::Iterable<T>(
            gen::begin<T>([iter, this]() mutable -> std::optional<T> {
                if (iter == this->_ns.end()) {
                    return {};

                } else if constexpr (is_raw_pointer<T>()) {
                    return &(iter++)->second->ref<T>();

                } else {
                    return (iter++)->second->get<T>();
                }
            }
        ));
    }

    unsigned int size() const {
        return _ns.size();
    }

    bool empty() const {
        return size() == 0;
    }

    std::vector<std::string> keys() const {
        return iterate_keys().collect();
    }

    template<class T>
    std::vector<T> values() const {
        return iterate_values<T>().collect();
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


}
}


#endif /* !__MOONLIGHT_JSON_OBJECT_H */
