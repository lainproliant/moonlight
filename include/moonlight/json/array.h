/*
 * array.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_ARRAY_H
#define __MOONLIGHT_JSON_ARRAY_H

#include "moonlight/json/core.h"
#include "moonlight/generator.h"

namespace moonlight {
namespace json {

class Array : public Value {
public:
    Array() : Value(Type::ARRAY) { }
    Array(const std::vector<Value::Pointer>& vec) : Value(Type::ARRAY), _vec(vec) { }
    virtual ~Array() { }

    template<class T>
    Array(const std::initializer_list<T>& init) : Array() {
        extend(init);
    }

    template<class T>
    Array(const std::vector<T>& vec) : Array() {
        std::transform(
            vec.begin(),
            vec.end(),
            std::back_inserter(_vec),
            [](const auto& v) { return Value::of(v); });
    }

    template<class T>
    Array value() const {
        return Array(_vec);
    }

    Value::Pointer clone() const override {
        return std::make_shared<Array>(_vec);
    }

    Array& clear() {
        _vec.clear();
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

    template<class T>
    Array& extend(const std::initializer_list<T>& init) {
        for (const auto& v : init) {
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
    gen::Iterator<T> begin() const {
        auto iter = _vec.begin();

        return gen::begin<T>([iter, this]() mutable -> std::optional<T> {
            if (iter == this->_vec.end()) {
                return {};

            } else if constexpr (is_raw_pointer<T>()) {
                return &(*(iter++))->ref<T>();

            } else {
                return (*(iter++))->get<T>();
            }
        });
    }

    template<class T>
    gen::Iterator<T> end() const {
        return gen::end<T>();
    }

    template<class T>
    gen::Stream<T> stream() const {
        return gen::Stream<T>(begin<T>());
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
            THROW(core::IndexError, std::to_string(offset));
        }
        return _vec.at(offset);
    }

    Value::Pointer& _back() {
        if (empty()) {
            THROW(core::IndexError, "Array is empty.");
        }
        return _vec.back();
    }

    std::vector<Value::Pointer> _vec;
};

template<>
inline Array::Array(const std::vector<Value::Pointer>& vec) : Value(Type::ARRAY), _vec(vec) { }

template<>
inline Value::Pointer Array::get(unsigned int offset) const {
    return _cget(offset);
}

VALUE_IS(Array, Type::ARRAY);
VALUE_OF(Array, value);
VALUE_GET(Array, Array);
VALUE_REF(Array);

}
}


#endif /* !__MOONLIGHT_JSON_ARRAY_H */
