/*
 * ## exceptions.h: Improved base exception types. ------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library offers a variety of exception base classes and macros that can
 * be used as the basis for a hierarchy of exception types in your programs.
 *
 * - `core::Exception`: A base exception type, which supports receiving a
 *   message, a `debug::Source` where the exception occurred, and preserves the
 *   name of the exception type through downcasts via a string.  This is best
 *   used as a base class for your exception hierarchy, and several macros exist
 *   to more easily facilitiate this, which are documented below.
 * - `EXCEPTION_TYPE(name)`: A macro for quickly declaring an exception type
 *   with `core::Exception` as its base.  The first constructor parameter for
 *   the resulting class will be an `std::string` message, and the second
 *   parameter is an optional `debug::Source` value, which will be automatically
 *   applied if you use the `THROW()` macro, see below.
 * - `EXCEPTION_SUBTYPE(base, name)`: A macro for quickly declaring an exception
 *   type that is a subtype of one of your base exception types.
 * - `THROW(excType, ...)`: A macro for throwing an exception, which
 *   automatically captures the source of the exception using the `LOCATION`
 *   macro, which expands to creating a `debug::Source` object using `__FILE__`,
 *   `__PRETTY_FUNCTION__` and `__LINE__`.  This macro assumes that the final
 *   parameter of the exception class constructor accepts a `debug::Source`
 *   value, which will be true for any exceptions declared using
 *   `EXCEPTION_TYPE()` or `EXCEPTION_SUBTYPE()`.  If you intend to use
 *   `THROW()` with your own manually defined exception types, it must accept a
 *   `debug::Source` as described above.
 * - `FAIL(...)`: A shortcut for using `THROW()` to throw a
 *   `core::AssertionFailure` error, useful for unit tests.
 *
 * A few common exception types are also declared in `exceptions.h` and used
 * throughout the `moonlight` meta library:
 *
 * - `AssertionFailure`: Mainly used for unit tests and runtime assertions to
 *   indicate that an expected condition has not been met.
 * - `ValueError`: Indicates that an invalid value has been provided.  Inspired
 *   by Python's `ValueError`.
 * - `IndexError`: Indicates that an invalid index has been provided.  Inspired
 *   by Python's `IndexError`.
 * - `RuntimeError`: Indicates a runtime issue with a continuous system.
 * - `UsageError`: Indicates incorrect usage of a function or idiom.
 * - `TypeError`: Indicates that an unexpected data type was encountered in data
 *   transmission, parsing, or decoding.
 * - `FrameworkError`: Indicates an bug with something in `moonlight` itself.
 *   Generally conditions that would cause `FrameworkError` to be thrown
 *   shouldn't occur, and if they do, it can be assumed to be a bug in `moonlight`.
 */

#ifndef __MOONLIGHT_EXCEPTIONS_H
#define __MOONLIGHT_EXCEPTIONS_H

#include <unistd.h>

#ifdef MOONLIGHT_ENABLE_STACKTRACE
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#endif

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>


#include "moonlight/debug.h"
#include "moonlight/string.h"
#include "moonlight/constants.h"
#include "moonlight/finally.h"

namespace moonlight {
namespace core {

/**
 * A generic base exception class.
 */
class Exception : public std::runtime_error {
 public:
     Exception(const std::string& message, debug::Source where = {},
               const std::string& type = type_name<Exception>())
     : std::runtime_error(message.c_str()), _type(type), _message(message),
#ifdef MOONLIGHT_ENABLE_STACKTRACE
     _stacktrace(debug::StackTrace::generate(where))
#else
     _stacktrace(where)
#endif
     {
         _full_message = full_message();
     }

     Exception(const Exception& e) : std::runtime_error(e.message().c_str()) {
         _type = e._type;
         _message = e._message;
         _stacktrace = e._stacktrace;
         if (e._cause != nullptr) {
             _cause = new Exception(*e._cause);
         }
         _full_message = full_message();
     }

     virtual ~Exception() {
         if (_cause != nullptr) {
             delete _cause;
         }
     }

     virtual const char* what() const throw() {
         return _full_message.c_str();
     }

     const std::string& type() const {
         return _type;
     }

     const debug::Source& where() const throw() {
         return _stacktrace.where();
     }

     virtual std::string full_message() const throw() {
         std::ostringstream sb;
         sb << type_and_message();

#ifdef MOONLIGHT_STACKTRACE_IN_DESCRIPTION
         sb << std::endl;
         if (! _stacktrace.contains_where() && ! where().is_nowhere()) {
             sb << "    from " << where();
         }
         _stacktrace.format(sb, "    at ");
#else
         if (! where().is_nowhere()) {
             sb << std::endl << "    from " << where();
         }
#endif

         if (has_cause()) {
             sb << std::endl;
             sb << "Caused by " << cause().full_message();
         }

         return sb.str();
     }

     std::string type_and_message() const {
         std::ostringstream sb;
         sb << type() << ": " << message();
         return sb.str();
     }

     const std::string& message() const {
         return _message;
     }

     Exception& caused_by(const Exception& cause) {
         _cause = new Exception(cause);
#ifdef MOONLIGHT_STACKTRACE_IN_DESCRIPTION
         // Need to update _full_message to reflect the cause.
         this->_full_message = full_message();
#endif
         return *this;
     }

     bool has_cause() const {
         return _cause != nullptr;
     }

     const Exception& cause() const {
         return *_cause;
     }

     friend std::ostream& operator<<(std::ostream& out, const Exception& e) {
         return out << e.full_message();
     }

 private:
     std::string _type;
     std::string _message;
     std::string _full_message;
     moonlight::debug::StackTrace _stacktrace;
     Exception* _cause = nullptr;
};

#define EXCEPTION_TYPE(Name) \
class Name : public moonlight::core::Exception { \
 public: \
         explicit Name(const std::string& message, \
                       const moonlight::debug::Source& loc = {}, \
                       const std::string& name = moonlight::type_name<Name>()) \
    : Exception(message, loc, name) { } \
};

#define EXCEPTION_SUBTYPE(Base, Name) \
class Name : public Base { \
 public: \
         Name(const std::string& message, \
              const moonlight::debug::Source& loc = {}, \
              const std::string& name = moonlight::type_name<Name>()) \
    : Base(message, loc, name) { } \
}

EXCEPTION_TYPE(AssertionFailure);
EXCEPTION_TYPE(ValueError);
EXCEPTION_TYPE(IndexError);
EXCEPTION_TYPE(RuntimeError);
EXCEPTION_TYPE(UsageError);
EXCEPTION_TYPE(TypeError);
EXCEPTION_TYPE(FrameworkError);

#define THROW(excType, ...) throw excType(__VA_ARGS__, LOCATION)
#define FAIL(...) THROW(moonlight::core::AssertionFailure, __VA_ARGS__)

}  // namespace core
}  // namespace moonlight


#endif /* !__MOONLIGHT_EXCEPTIONS_H */
