/*
 * result.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday June 15, 2025
 */

#ifndef __MOONLIGHT_RESULT_H
#define __MOONLIGHT_RESULT_H

#include <optional>
#include <string>
#include <sstream>

#ifdef MOONLIGHT_ENABLE_STACKTRACE
#include "moonlight/debug.h"
#endif

namespace moonlight {

// ------------------------------------------------------------------
template<class T = int>
class Error {
public:
#ifdef MOONLIGHT_ENABLE_STACKTRACE
    Error(const debug::StackTrace& stack)
    : _stack(stack) { }
    Error(const debug::StackTrace& stack, const T& value)
    : _value(value), _stack(stack) { }
#else
    Error() { }
    Error(const T& value)
    : _value(value) { }
#endif

    Error<T>& operator()(const T& value) {
        _value = value;
        return *this;
    }

    const T& value() const {
        return _value;
    }

    std::string str() const {
        std::ostringstream sb;
        sb << value() << std::endl;
#ifdef MOONLIGHT_ENABLE_STACKTRACE
        stack().format(sb, "    at ");
#endif
        return sb.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const Error& e) {
        out << e.str();
        return out;
    }

#ifdef MOONLIGHT_ENABLE_STACKTRACE
    const debug::StackTrace& stack() const {
        return _stack;
    }
#endif

private:
    T _value;

#ifdef MOONLIGHT_ENABLE_STACKTRACE
    debug::StackTrace _stack;
#endif

};

// ------------------------------------------------------------------
#ifdef MOONLIGHT_ENABLE_STACKTRACE
#define ERROR Error(debug::StackTrace::generate(LOCATION))
#else
#define ERROR Error()
#endif

// ------------------------------------------------------------------
template<class T, class E>
class Result {
 public:
     Result()
     : _value(T()) { }

     Result(const T& value)
     : _value(value) { }

     Result(const Error<E>& error)
     : _error(error) { }

     bool ok() const {
         return _value.has_value();
     }

     const T& value() const {
         return _value.value();
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
