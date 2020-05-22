/*
 * moonlight/core.h: Standalone essentials for everyday coding.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Thursday, Dec 21 2017
 */
#pragma once

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <exception>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef MOONLIGHT_ENABLE_STACKTRACE
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#endif

//-------------------------------------------------------------------
#define ALIAS_TYPEDEFS(T) \
typedef std::shared_ptr<T> pointer; \
typedef std::shared_ptr<const T> const_pointer; \
typedef std::vector<pointer> list; \
typedef std::vector<const_pointer> const_list;

//-------------------------------------------------------------------
#ifdef MOONLIGHT_DEBUG
#include <csignal>
#define debugger raise(SIGSEGV)
#else
#define debugger (void)0
#endif

//-------------------------------------------------------------------
#ifdef MOONLIGHT_DEBUG
#define moonlight_assert(x) \
   if (! (x)) { \
      throw moonlight::core::AssertionFailure(#x, __FUNCTION__, __FILE__, __LINE__); \
   }
#else
#define moonlight_assert(x) (void)0;
#endif

//-------------------------------------------------------------------
template<typename T, typename... TD>
std::shared_ptr<T> make(TD&&... params) {
   return std::make_shared<T>(std::forward<TD>(params)...);
}

namespace moonlight {
namespace variadic {
/**------------------------------------------------------------------
 * An empty base structure used for iterating across variadic
 * template parameters in order.  Takes advantage of the fact
 * that initialization lists preserve order of parameters
 * when variadic template parameters are expanded in them.
 *
 * ```
 * template<typename T, typename... TV>
 * T sum(T init, const TV&&... values) {
 *    T total = init;
 *    variadic::pass{(total += values)...};
 * }
 * ```
 *
 * To use with void function calls or statements, provide
 * a second statement that yields a value, e.g. a const statement:
 *
 * ```
 * template<typename T, typename... TV>
 * void foreach(std::function<void(T)> f, const TV&&... values) {
 *    variadic::pass{(f(values), 0)...};
 * }
 * ```
 */
struct pass {
   template<typename... T> pass(T...) { }
};

//-------------------------------------------------------------------
template<typename T, typename... TD>
inline void _to_vector(std::vector<T>& vec) {
   (void)vec;
}

//-------------------------------------------------------------------
template<typename T, typename... TD>
inline void _to_vector(std::vector<T>& vec, const T& item, const TD&... items) {
   vec.push_back(item);
   _to_vector(vec, items...);
}

//-------------------------------------------------------------------
template<typename T, typename... TD>
inline std::vector<T> to_vector(const T& item, const TD&... items) {
   std::vector<T> vec;
   _to_vector(vec, item, items...);
   return vec;
}
}

namespace str {
inline void _cat(std::ostringstream& sb) {
   (void) sb;
}

template<typename T, typename... TD>
inline void _cat(std::ostringstream& sb, const T& element, const TD&... elements) {
   sb << element;
   _cat(sb, elements...);
}

template<typename T, typename... TD>
inline std::string cat(const T element, const TD&... elements) {
   std::ostringstream sb;
   _cat(sb, element, elements...);
   return sb.str();
}

/**------------------------------------------------------------------
 * Determine if one string starts with another's characters.
 */
inline bool startswith(const std::string& str,
                       const std::string& prefix) {
   return ! str.compare(0, prefix.size(), prefix);
}

/**------------------------------------------------------------------
 * Determine if one string ends with another's characters.
 */
inline bool endswith(const std::string& str,
                     const std::string& suffix) {
   return suffix.length() <= str.length() &&
   ! str.compare(str.length() - suffix.length(),
                 suffix.length(), suffix);
}

/**------------------------------------------------------------------
 * Join the given iterable collection into a token delimited string.
 */
template<typename T>
inline std::string join(const T& coll, const std::string& token = "") {
   std::ostringstream sb;

   for (typename T::const_iterator i = coll.begin(); i != coll.end();
        i++) {
      if (i != coll.begin()) sb << std::string(token);
      sb << *i;
   }

   return sb.str();
}

/**------------------------------------------------------------------
 * Split the given string into a list of strings based on
 * the delimiter provided, and insert them into the given
 * collection.  Collection must support push_back().
 *
 * @param tokens The collection into which the split string
 *    elements will be appended.
 */
template <typename T>
inline void split(T& tokens, const std::string& s, const std::string& delimiter) {
   std::string::size_type from = 0;
   std::string::size_type to = 0;

   while (to != std::string::npos) {
      to = s.find(delimiter, from);
      if (from < s.size() && from != to) {
         tokens.push_back(s.substr(from, to - from));
      }

      from = to + delimiter.size();
   }
}

/**------------------------------------------------------------------
 * Split the given string into a list of strings based on
 * the delimiter provided.
 *
 * @return The contents of the string, minus the delimiters,
 *    split along the delimiter boundaries in a linked list.
 */
inline std::vector<std::string> split(const std::string& s, const std::string&
                                      delimiter) {
   std::vector<std::string> tokens;
   split(tokens, s, delimiter);
   return tokens;
}

/**------------------------------------------------------------------
 * Create a string consisting of a single character.
 */
inline std::string chr(char c) {
   std::string str;
   str.push_back(c);
   return str;
}

/**------------------------------------------------------------------
 * Trim all whitespace from the left.
 */
inline std::string trim_left(const std::string& s) {
   std::string copy = s;
   copy.erase(copy.begin(),
              std::find_if(copy.begin(),
                           copy.end(),
                           [](int c) { return !isspace(c); }));
   return copy;
}

/**------------------------------------------------------------------
 * Trim all whitespace from the right.
 */
inline std::string trim_right(const std::string& s) {
   std::string copy = s;
   copy.erase(
       std::find_if(
           copy.rbegin(),
           copy.rend(),
           [](int c) { return !isspace(c); }
       ).base(),
       copy.end());
   return copy;
}

/**------------------------------------------------------------------
 * Trim all whitespace from the left or right.
 */
inline std::string trim(const std::string& s) {
   return trim_left(trim_right(s));
}
}

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

namespace splice {
template<typename T>
//-------------------------------------------------------------------
inline std::optional<typename T::value_type> at(const T& coll, int offset) {
   if (offset < 0) {
      offset += coll.size();
   }

   if (offset >= coll.size()) {
      return {};
   } else {
      return coll[offset];
   }
}

//-------------------------------------------------------------------
template<typename T>
inline T from_to(const T& coll, int begin_offset, int end_offset) {
   T spliced;

   if (begin_offset < 0) {
      begin_offset += coll.size();
   }

   if (end_offset < 0) {
      end_offset += coll.size();
   }

   if (end_offset > coll.size()) {
      end_offset = coll.size();
   }

   for (int x = begin_offset; x < end_offset && x < coll.size(); x++) {
      spliced.push_back(coll[x]);
   }

   return spliced;
}

//-------------------------------------------------------------------
template<typename T>
inline T begin(const T& coll, int begin_offset) {
   return from_to(coll, begin_offset, coll.size());
}

//-------------------------------------------------------------------
template<typename T>
inline T end(const T& coll, int end_offset) {
   return from_to(coll, 0, end_offset);
}
}

namespace maps {
/**------------------------------------------------------------------
 * Copy a vector of the keys from the given map-like iterable.
 */
template<typename T>
inline std::vector<typename T::key_type> keys(const T& map) {
   std::vector<typename T::key_type> kvec;

   for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
      kvec.push_back(iter->first);
   }

   return kvec;
}

/**------------------------------------------------------------------
 * Copy a vector of the values from the given map-like iterable.
 */
template<typename T>
inline std::vector<typename T::mapped_type> values(const T& map) {
   std::vector<typename T::mapped_type> vvec;

   for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
      vvec.push_back(iter->second);
   }

   return vvec;
}

template<typename T>
inline const typename T::mapped_type& const_value(const T& map, const typename T::key_type& key) {
   auto iter = map.find(key);
   if (iter != map.end()) {
      return iter->second;
   } else {
      throw core::IndexException("Key does not exist in map.");
   }
}
}

namespace mmaps {
/**------------------------------------------------------------------
 * Template class used as parameter type for mmaps::build().
 */
template<typename K, typename T>
struct mapping {
   K key;
   std::vector<T> values;
};

/**------------------------------------------------------------------
 * Build a multimap from the given constant mapping.  Intended for use
 * in statically defining multimaps, e.g.:
 *
 * ```
 * static const auto mmap = build({
 *    {"even",    {2, 4, 6, 8}},
 *    {"odd",     {1, 3, 5, 7}}
 * });
 * ```
 */
template<typename K, typename T>
inline std::multimap<K, T> build(const std::vector<mapping<K, T>>& mappings) {
   std::multimap<K, T> mmap;
   for (auto mapping : mappings) {
      for (auto value : mapping.values) {
         mmap.insert(std::make_pair(mapping.key, value));
      }
   }

   return mmap;
}

/**------------------------------------------------------------------
 * Variadic version of mmap::build().
 *
 * ```
 * static const auto mmap = build(
 *    "even",    {2, 4, 6, 8},
 *    "odd",     {1, 3, 5, 7}
 * );
 * ```
 */
template<typename K, typename T, typename... KTV>
inline std::multimap<K, T> build(const K& key, const T& value, const KTV&... mappings) {
   std::multimap<K, T> mmap;
   build(mmap, key, value, mappings...);
   return mmap;
}

//-------------------------------------------------------------------
template<typename K, typename T, typename... KTV>
inline void build(std::multimap<K, T>& mmap, const K& key, const T& value,
                  const KTV&... mappings) {
   mmap.insert({key, value});
   build(mmap, mappings...);
}

template<typename K, typename T>
inline void build(std::multimap<K, T>& mmap) { (void) mmap; }

/**------------------------------------------------------------------
 * Collect all values from the given multimap-like iterable that match
 * the given key.
 */
template<typename M>
inline std::vector<typename M::mapped_type> collect(const M& mmap,
                                                    const typename M::key_type& key) {
   std::vector<typename M::mapped_type> values;
   auto range = mmap.equal_range(key);

   for (auto iter = range.first; iter != range.second; iter++) {
      values.insert(values.end(), iter->second);
   }

   return values;
}
}

namespace collect {
//-------------------------------------------------------------------
template<typename T>
inline bool contains(const T& coll, const typename T::value_type& v) {
   return std::find(coll.begin(), coll.end(), v) != coll.end();
}

//-------------------------------------------------------------------
template<typename T>
inline bool contains(const std::set<T>& set, const T& v) {
   return set.find(v) != set.end();
}

//-------------------------------------------------------------------
template<typename K, typename V>
inline bool contains(const std::map<K, V>& map, const K& v) {
   return map.find(v) != map.end();
}

//-------------------------------------------------------------------
template<typename T, typename... TV>
inline std::vector<typename T::value_type> flatten(const T& coll, const TV&... collections) {
   std::vector<typename T::value_type> flattened;
   flatten(flattened, coll, collections...);
   return flattened;
}

//-------------------------------------------------------------------
template<typename T, typename... TV>
inline void flatten(std::vector<typename T::value_type>& flattened, const T& coll, const TV&... collections) {
   for (auto v : coll) {
      flattened.push_back(v);
   }
   flatten(flattened, collections...);
}

//-------------------------------------------------------------------
template<typename T>
inline void flatten(std::vector<T>& flattened) { (void) flattened; }

//-------------------------------------------------------------------
template<typename T>
inline T filter(const T& coll, const std::function<bool(typename T::value_type)>& f) {
   T result;
   for (auto v : coll) {
      if (f(v)) {
         result.push_back(v);
      }
   }
   return result;
}

//-------------------------------------------------------------------
template<typename T>
inline std::shared_ptr<T> filter(const std::shared_ptr<T>& coll,
                                 const std::function<bool(typename T::value_type)>& f) {
   return std::make_shared<T>(filter<typename std::shared_ptr<T>::element_type>(*coll, f));
}

//-------------------------------------------------------------------
template<typename C1>
inline C1 sorted(const C1& src) {
   C1 result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   std::sort(result.begin(), result.end());
   return result;
}

//-------------------------------------------------------------------
template<typename C1>
inline C1 sorted(const C1& src, std::function<bool (const typename C1::value_type& a,
                                                    const typename C1::value_type& b)> comp) {
   C1 result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   std::sort(result.begin(), result.end(), comp);
   return result;
}

//-------------------------------------------------------------------
template<typename T, typename C1>
inline std::vector<T> map(const C1& src, std::function<T (const typename C1::value_type&)> f) {
   std::vector<T> result;
   for (auto v : src) {
      result.push_back(f(v));
   }
   return result;
}

//-------------------------------------------------------------------
template<typename C>
inline std::set<typename C::value_type> set(const C& src) {
   std::set<typename C::value_type> result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   return result;
}
}

namespace file {
//-------------------------------------------------------------------
class FileException : public core::Exception {
   using core::Exception::Exception;
};

//-------------------------------------------------------------------
inline std::ifstream open_r(const std::string& filename,
                            std::ios::openmode mode = std::ios::in) {
   try {
      std::ifstream infile;
      infile.exceptions(std::ios::failbit);
      infile.open(filename, mode);
      infile.exceptions(std::ios::badbit);
      return infile;

   } catch (std::exception& e) {
      std::ostringstream sb;
      sb << "Cannot open file " << filename << " for reading: "
      << strerror(errno);
      throw FileException(sb.str());
   }
}

//-------------------------------------------------------------------
inline std::ofstream open_w(const std::string& filename,
                            std::ios::openmode = std::ios::out) {
   try {
      std::ofstream outfile;
      outfile.exceptions(std::ios::failbit);
      outfile.open(filename);
      outfile.exceptions(std::ios::badbit);
      return outfile;

   } catch (std::exception& e) {
      std::ostringstream sb;
      sb << "Cannot open file " << filename << " for writing: "
      << strerror(errno);
      throw FileException(sb.str());
   }
}

//-------------------------------------------------------------------
inline std::fstream open_rw(const std::string& filename,
                            std::ios::openmode mode = std::ios::in | std::ios::out) {
   try {
      std::fstream outfile;
      outfile.exceptions(std::ios::failbit);
      outfile.open(filename, mode);
      outfile.exceptions(std::ios::badbit);
      return outfile;

   } catch (std::exception& e) {
      std::ostringstream sb;
      sb << "Cannot open file " << filename << " for reading and writing: "
      << strerror(errno);
      throw FileException(sb.str());
   }
}

//-------------------------------------------------------------------
inline std::string to_string(std::istream& infile) {
   return std::string(std::istreambuf_iterator<char>(infile), {});
}

//-------------------------------------------------------------------
inline std::string to_string(const std::string& filename) {
   auto infile = open_r(filename);
   return to_string(infile);
}

//-------------------------------------------------------------------
// (namely an alias)
inline std::string slurp(const std::string& filename) {
   return to_string(filename);
}

//-------------------------------------------------------------------
inline void dump(const std::string& filename, const std::string& str) {
   auto outfile = open_w(filename);
   outfile << str;
}

//-------------------------------------------------------------------
template<class T>
class Writable {
public:
   virtual std::string repr() const = 0;
   friend std::ostream& operator<<(std::ostream& out, const T& obj) {
      out << obj.repr();
      return out;
   }
};

// ------------------------------------------------------------------
class BufferedInput {
public:
    BufferedInput(std::istream& input, const std::string& name = "<input>")
    : _input(input), _name(name) { }

    int getc() {
        int c;

        if (_buffer.size() > 0) {
            c = _buffer[0];
            for (size_t x = 1; x < _buffer.size(); x++) {
               _buffer[x-1] = _buffer[x];
            }
            _buffer.pop_back();

        } else {
            c = _input.get();
        }

        if (c == '\n') {
            _line ++;
            _col = 1;

        } else if (c == EOF) {
            _exhausted = true;

        } else {
            _col ++;
        }

        return c;
    }

    bool is_exhausted() const {
        return _exhausted;
    }

    int peek(size_t offset = 1) {
        if (offset == 0) {
            return EOF;
        }

        while (_buffer.size() < offset) {
            int c = _input.get();
            if (c == EOF) {
                return EOF;
            }
            _buffer.push_back(c);
        }

        return _buffer[offset-1];
    }

    void advance(size_t offset = 1) {
       for (size_t x = 0; x < offset; x++) {
          getc();
       }
    }

    bool scan_eq(const std::string& target) {
       for (size_t x = 0; x < target.size(); x++) {
          if (peek(x+1) != target[x]) {
             return false;
          }
       }

       return true;
    }

    std::string scan_dump() {
       std::string dump;
       for (size_t x = 1; peek(x) != EOF; x++) {
          dump.push_back(peek(x));
       }
       return dump;
    }

    const std::string& name() const {
        return _name;
    }

    int line() const {
        return _line;
    }

    int col() const {
        return _col;
    }

private:
    std::istream& _input;
    const std::string _name;
    int _line = 1, _col = 1;
    bool _exhausted = false;
    std::string _buffer;
};

}
}
