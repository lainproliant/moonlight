/*
 * exceptions.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_EXCEPTIONS_H
#define __MOONLIGHT_EXCEPTIONS_H

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

#ifdef MOONLIGHT_ENABLE_STACKTRACE
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#endif

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

	virtual const char* what() const throw()
	{
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
        Name(const std::string& message, const moonlight::debug::Source& loc = {}, const std::string& name = moonlight::type_name<Name>()) : Exception(message, loc, name) { } \
    };

#define EXCEPTION_SUBTYPE(Base, Name) \
    class Name : public Base { \
    public: \
        Name(const std::string& message, const moonlight::debug::Source& loc = {}, const std::string& name = moonlight::type_name<Name>()) : Base(message, loc, name) { } \
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

}
}


#endif /* !__MOONLIGHT_EXCEPTIONS_H */
