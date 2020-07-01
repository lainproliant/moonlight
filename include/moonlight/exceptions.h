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

#include "moonlight/string.h"
#include <functional>
#include <sstream>
#include <stdexcept>

#ifdef MOONLIGHT_ENABLE_STACKTRACE
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#endif

namespace moonlight {
namespace core {

/**------------------------------------------------------------------
 * An object to facilitiate finalization when not in the
 * scope of an object.  For example, this object can be used
 * to ensure that a function will be called when a scope
 * is exited, whether it left through the normal flow of
 * execution or via an exception.
 *
 * Note that this will not be called for POSIX signals
 * which interrupt execution.
 *
 * Example Usage:
 *
 * void perform_task(promise<string>& string_promise,
 *                   function<void()> user_defined_task) {
 *
 *    bool success = false;
 *    string failure_reason = "i don't know";
 *
 *    Finalizer finally([&]() {
 *       if (success) {
 *          string_promise.set_value("It's done!");
 *
 *       } else {
 *          string_promise.set_value("It failed, because " +
 *             failure_reason);
 *       }
 *    });
 *
 *    try {
 *       user_defined_task();
 *       success = true;
 *    } catch (const Exception& e) {
 *       failure_reason = e.get_message();
 *    }
 * }
 */
class Finalizer {
public:
   Finalizer(std::function<void()> closure) : closure(closure) { }
   virtual ~Finalizer() {
      closure();
   }

private:
   std::function<void()> closure;
};

//-------------------------------------------------------------------
inline std::vector<std::string> generate_stacktrace(int max_frames = 256) {
   (void) max_frames;
#ifdef MOONLIGHT_ENABLE_STACKTRACE
#define MOONLIGHT_STACKTRACE_LINE_BUFSIZE 1024
   std::vector<std::string> btvec;
   char buf[MOONLIGHT_STACKTRACE_LINE_BUFSIZE];
   void* callstack[max_frames];
   char** symbols;
   size_t num_frames = backtrace(callstack, max_frames);
   symbols = backtrace_symbols(callstack, num_frames);
   for (int x = 1; x < num_frames; x++) {
      Dl_info info;
      if (dladdr(callstack[x], &info) && info.dli_sname) {
         char* demangled = nullptr;
         int status = -1;
         if (info.dli_sname[0] == '_') {
            demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);
            snprintf(buf, MOONLIGHT_STACKTRACE_LINE_BUFSIZE, "[%3d] %*p %s +%zd",
                     x, int(2 + sizeof(void*) * 2), callstack[x],
                     status == 0 ? demangled :
                     info.dli_sname == 0 ? symbols[x] : info.dli_sname,
                     (char *)callstack[x] - (char *)info.dli_saddr);
            free(demangled);
         }
      } else {
         snprintf(buf, MOONLIGHT_STACKTRACE_LINE_BUFSIZE, "[%3d] %*p %s", x,
                  int(2 + sizeof(void*) * 2), callstack[x],
                  symbols[x]);
      }

      btvec.push_back(std::string(buf));
   }
#else
   std::vector<std::string> btvec(0);
#endif

   return btvec;
}

//-------------------------------------------------------------------
inline std::string format_stacktrace(const std::vector<std::string>& stacktrace) {
   std::ostringstream sb;
   sb << "Stack Trace --> \n\t" << str::join(stacktrace, "\n\t");
   return sb.str();
}

/**------------------------------------------------------------------
 * A generic base class for throwables that may or may not be
 * runtime exceptions.
 */
class Throwable { };

/**------------------------------------------------------------------
 * A generic base exception class.
 */
class Exception : public std::runtime_error, Throwable {
public:
   Exception(const std::string& message) : std::runtime_error(message.c_str()) {
      stacktrace = generate_stacktrace();
#ifdef MOONLIGHT_STACKTRACE_IN_DESCRIPTION
      std::ostringstream sb;
      sb << message << "\n" << format_stacktrace(stacktrace) << "\n";
      this->message = sb.str();
#else
      this->message = message;
#endif
   }

   virtual const char* what() const throw()
   {
      return message.c_str();
   }

   virtual const std::string& get_message() const {
      return message;
   }

private:
   std::string message;
   std::vector<std::string> stacktrace;
};

//-------------------------------------------------------------------
class IndexException : public Exception {
   using Exception::Exception;
};

//-------------------------------------------------------------------
class NotImplementedException : public Exception {
   using Exception::Exception;
};

//-------------------------------------------------------------------
class AssertionFailure : public core::Exception {
public:
   AssertionFailure(const std::string& expression,
                    const std::string& function,
                    const std::string& filename,
                    int line) : Exception(_construct(expression, function, filename, line)) { }

   static std::string _construct(const std::string& expression,
                                 const std::string& function,
                                 const std::string& filename,
                                 int line) {

      std::stringstream sb;
      sb << "Assertion failed: "
         << "\"" << expression
         << " @ " << function << " (" << filename << ":" << line << ")";
      return sb.str();
   }
};


}
}


#endif /* !__MOONLIGHT_EXCEPTIONS_H */
