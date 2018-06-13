/*
 * moonlight/core.h: Standalone essentials for everyday coding.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Thursday, Dec 21 2017
 */
#pragma once

#include <algorithm>
#include <cassert>
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
#endif

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
template<class T, class... TD>
void _to_vector(std::vector<T>& vec) {
   (void)vec;
}

//-------------------------------------------------------------------
template<class T, class... TD>
void _to_vector(std::vector<T>& vec, const T& item, const TD&... items) {
   vec.push_back(item);
   _to_vector(vec, items...);
}

//-------------------------------------------------------------------
template<class T, class... TD>
std::vector<T> to_vector(const T& item, const TD&... items) {
   std::vector<T> vec;
   _to_vector(vec, item, items...);
   return vec;
}
}

namespace str {
/**------------------------------------------------------------------
 * Concatenate a list of strings together.
 */
std::string cat(const std::vector<std::string>& strings) {
   std::ostringstream sb;
   for (auto s : strings) {
      sb << s;
   }
   return sb.str();
}

/**------------------------------------------------------------------
 * Variadic variant of str::cat() that accepts all types
 * which can be shifted into an output stream.
 */
template<typename T, typename... TD>
std::string cat(const T element, const TD&... elements) {
   return cat(variadic::to_vector(element, elements...));
}

/**------------------------------------------------------------------
 * Determine if one string starts with another's characters.
 */
bool startswith(const std::string& str,
                const std::string& prefix) {
   return ! str.compare(0, prefix.size(), prefix);
}

/**------------------------------------------------------------------
 * Determine if one string ends with another's characters.
 */
bool endswith(const std::string& str,
              const std::string& suffix) {
   return suffix.length() <= str.length() &&
   ! str.compare(str.length() - suffix.length(),
                 suffix.length(), suffix);
}

/**------------------------------------------------------------------
 * Join the given iterable collection into a token delimited string.
 */
template<typename T>
std::string join(const T& coll, const std::string& token = "") {
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
void split(T& tokens, const std::string& s, const std::string& delimiter) {
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
std::vector<std::string> split(const std::string& s,
                               const std::string& delimiter) {
   std::vector<std::string> tokens;
   split(tokens, s, delimiter);
   return tokens;
}

/**------------------------------------------------------------------
 * Create a string consisting of a single character.
 */
std::string chr(char c) {
   std::string str;
   str.push_back(c);
   return str;
}

/**------------------------------------------------------------------
 * Trim all whitespace from the left.
 */
std::string trim_left(const std::string& s) {
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
std::string trim_right(const std::string& s) {
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
std::string trim(const std::string& s) {
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
std::vector<std::string> generate_stacktrace(int max_frames = 256) {
   (void) max_frames;
#ifdef MOONLIGHT_ENABLE_STACKTRACE
   void* frames[max_frames];
   char** formatted_frames;
   size_t num_frames = backtrace(frames, max_frames);
   std::vector<string> btvec;
   formatted_frames = backtrace_symbols(frames, num_frames);
   for (size_t x = 0; x < num_frames; x++) {
      string formatted_frame = string(formatted_frames[x]);
      if (formatted_frame.size() > 0) {
         btvec.push_back(formatted_frame);
      }
   }
#else
   std::vector<std::string> btvec(0);
#endif

   return btvec;
}

//-------------------------------------------------------------------
std::string format_stacktrace(const std::vector<std::string>& stacktrace) {
   std::ostringstream sb;
   sb << "Stack Trace --> \n\t" << str::join(stacktrace, "\n\t");
   return sb.str();
}

/**------------------------------------------------------------------
 * A generic base exception class.
 */
class Exception : public std::runtime_error {
public:
   Exception(const std::string& message) : std::runtime_error(message.c_str()) {
      stacktrace = generate_stacktrace();
#ifdef LAIN_STACKTRACE_IN_DESCRIPTION
      ostringstream sb;
      sb << message << "\n" << format_stacktrace() << "\n";
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

   std::string format_stacktrace() const {
      std::ostringstream sb;

      for (size_t x = 0; x < stacktrace.size(); x++) {
         sb << x << ": " << stacktrace[x] << std::endl;
      }

      return sb.str();
   }

   void print_stacktrace(std::ostream& out) const {
      out << format_stacktrace();
   }

   const std::vector<std::string>& get_stacktrace() const {
      return stacktrace;
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
}

namespace splice {
template<typename T>
//-------------------------------------------------------------------
std::optional<typename T::value_type> at(const T& coll, int offset) {
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
T from_to(const T& coll, int begin_offset, int end_offset) {
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
T begin(const T& coll, int begin_offset) {
   return from_to(coll, begin_offset, coll.size());
}

//-------------------------------------------------------------------
template<typename T>
T end(const T& coll, int end_offset) {
   return from_to(coll, 0, end_offset);
}
}

namespace maps {
/**------------------------------------------------------------------
 * Copy a vector of the keys from the given map-like iterable.
 */
template<typename T>
std::vector<typename T::key_type> keys(const T& map) {
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
std::vector<typename T::mapped_type> values(const T& map) {
   std::vector<typename T::mapped_type> vvec;

   for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
      vvec.push_back(iter->second);
   }

   return vvec;
}

template<typename T>
const typename T::mapped_type& const_value(const T& map, const typename T::key_type& key) {
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
std::multimap<K, T> build(const std::vector<mapping<K, T>>& mappings) {
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
std::multimap<K, T> build(const K& key, const T& value, const KTV&... mappings) {
   std::multimap<K, T> mmap;
   build(mmap, key, value, mappings...);
   return mmap;
}

//-------------------------------------------------------------------
template<typename K, typename T, typename... KTV>
void build(std::multimap<K, T>& mmap, const K& key, const T& value, 
           const KTV&... mappings) {
   mmap.insert({key, value});
   build(mmap, mappings...);
}

template<typename K, typename T>
void build(std::multimap<K, T>& mmap) { (void) mmap; }

/**------------------------------------------------------------------
 * Collect all values from the given multimap-like iterable that match
 * the given key.
 */
template<typename M>
std::vector<typename M::mapped_type> collect(const M& mmap,
                                             const typename M::key_type& key) {
   std::vector<typename M::mapped_type> values;
   auto range = mmap.equal_range(key);

   for (auto iter = range.first; iter != range.second; iter++) {
      values.push_back(iter->second);
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
std::vector<typename T::value_type> flatten(const T& coll, const TV&... collections) {
   std::vector<typename T::value_type> flattened;
   flatten(flattened, coll, collections...);
   return flattened;
}

//-------------------------------------------------------------------
template<typename T, typename... TV>
void flatten(std::vector<typename T::value_type>& flattened, const T& coll, const TV&... collections) {
   for (auto v : coll) {
      flattened.push_back(v);
   }
   flatten(flattened, collections...);
}

//-------------------------------------------------------------------
template<class T>
void flatten(std::vector<T>& flattened) { (void) flattened; }

//-------------------------------------------------------------------
template<class T>
T filter(const T& coll, const std::function<bool(typename T::value_type)>& f) {
   T result;
   for (auto v : coll) {
      if (f(v)) {
         result.push_back(v);
      }
   }
   return result;
}

//-------------------------------------------------------------------
template<class T>
std::shared_ptr<T> filter(const std::shared_ptr<T>& coll,
                          const std::function<bool(typename T::value_type)>& f) {
   return std::make_shared<T>(filter<typename std::shared_ptr<T>::element_type>(*coll, f));
}

//-------------------------------------------------------------------
template<class C1>
C1 sorted(const C1& src) {
   C1 result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   std::sort(result.begin(), result.end());
   return result;
}

//-------------------------------------------------------------------
template<class C1>
C1 sorted(const C1& src, std::function<bool (const typename C1::value_type& a,
                                             const typename C1::value_type& b)> comp) {
   C1 result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   std::sort(result.begin(), result.end(), comp);
   return result;
}

//-------------------------------------------------------------------
template<class T, class C1>
std::vector<T> map(const C1& src, std::function<T (const typename C1::value_type&)> f) {
   std::vector<T> result;
   for (auto v : src) {
      result.push_back(f(v));
   }
   return result;
}
}

namespace file {
//-------------------------------------------------------------------
class FileException : public core::Exception {
   using core::Exception::Exception;
};

//-------------------------------------------------------------------
std::ifstream open_r(const std::string& filename,
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
std::ofstream open_w(const std::string& filename,
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
std::fstream open_rw(const std::string& filename,
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
std::string to_string(std::istream& infile) {
   return std::string(std::istreambuf_iterator<char>(infile), {});
}

//-------------------------------------------------------------------
std::string to_string(const std::string& filename) {
   auto infile = open_r(filename);
   return to_string(infile);
}

}

}

