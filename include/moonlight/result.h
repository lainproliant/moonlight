/*
 * result.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday June 15, 2025
 */

#ifndef __MOONLIGHT_RESULT_H
#define __MOONLIGHT_RESULT_H

#include <optional>

namespace moonlight {

template<class T, class E>
class Result {
 public:
     Result(const T& value)
     : _value(value) { }

     Result(const E& error)
     : _error(error) { }

     bool has_value() const {
         return _value.has_value();
     }

     const T& value() const {
         return _value.value();
     }

     bool is_error() const {
         return _error.has_value();
     }

     const E& error() const {
         return _error.value();
     }

 private:
     std::optional<T> _value;
     std::optional<E> _error;
};

}

#endif /* !__MOONLIGHT_RESULT_H */
