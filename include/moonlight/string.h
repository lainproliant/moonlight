/*
 * string.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_STRING_H
#define __MOONLIGHT_STRING_H

#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include "moonlight/traits.h"

namespace moonlight {

namespace str {

template<typename T>
inline std::string coerce(const T& value) {
    std::ostringstream sb;

    if constexpr (is_streamable<T>()) {
        sb << value;

    } else {
        sb << "<" << type_name<T>() << ">";
    }

    return sb.str();
}

template<>
inline std::string coerce(const std::string& value) {
    return value;
}

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

      if (from != to) {
         tokens.push_back(s.substr(from, to - from));
      } else if (from == to) {
         tokens.push_back("");
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
inline std::vector<std::string> split(const std::string& s,
                                      const std::string& delimiter) {
   std::vector<std::string> tokens;
   split(tokens, s, delimiter);
   return tokens;
}

/**
 * Split the given string into a list of strings based on the
 * single character delimiter provided, with escape ('\\' by default)
 */
inline std::vector<std::string> split(const std::string& s, int delim, int escape = '\\') {
    std::vector<std::string> tokens;
    std::string token;

    for (size_t x = 0; x < s.size(); x++) {
        auto c = s[x];
        if (c == escape) {
            x++;
            continue;
        }
        if (c == delim) {
            tokens.push_back(token);
            token.clear();
        } else {
            token.push_back(c);
        }
    }

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

/**------------------------------------------------------------------
 * Trim a string from the beginning of another if present.
 */
inline std::string trim_prefix(const std::string& prefix, const std::string& s) {
    if (s.starts_with(prefix)) {
        std::string s_copy = s;
        s_copy.erase(0, prefix.size());
        return s_copy;
    }

    return s;
}

/**------------------------------------------------------------------
 * Apply a function to each character in the string.
 */
inline std::string map(const std::string& s, std::function<char(char)> transformer) {
   std::string result;
   std::transform(s.begin(), s.end(), std::back_inserter(result), transformer);
   return result;
}

/**------------------------------------------------------------------
 * Apply `toupper` to each character in the string.
 */
inline std::string to_upper(const std::string& s) {
   return map(s, [](char c) { return toupper(c); });
}

/**------------------------------------------------------------------
 * Apply `tolower` to each character in the string.
 */
inline std::string to_lower(const std::string& s) {
   return map(s, [](char c) { return tolower(c); });
}

// ------------------------------------------------------------------
inline std::string literal(const std::string& str) {
    static const std::map<char, std::string> ESCAPE_SEQUENCES = {
        {'\a', "\\a"},
        {'\b', "\\b"},
        {'\e', "\\e"},
        {'\f', "\\f"},
        {'\n', "\\n"},
        {'\r', "\\r"},
        {'\t', "\\t"},
        {'\v', "\\v"},
        {'\\', "\\\\"},
        {'"', "\\\""}
    };

    std::stringstream sb;

    for (size_t x = 0; str[x] != '\0'; x++) {
        char c = str[x];
        std::string repr = "";

        auto iter = ESCAPE_SEQUENCES.find(c);
        if (iter != ESCAPE_SEQUENCES.end()) {
            repr = iter->second;
        }

        if (repr == "" && !isprint(c)) {
            std::ios sb_state(nullptr);
            sb_state.copyfmt(sb);
            sb << std::hex << std::setfill('0') << std::setw(2);
            sb << "\\x" << (0xFF & c);
            sb.copyfmt(sb_state);

        } else if (repr != "") {
            sb << repr;
        } else {
            sb << c;
        }
    }

    return sb.str();
}

}
}

#endif /* !__MOONLIGHT_STRING_H */
