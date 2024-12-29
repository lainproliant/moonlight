/*
 * ## rx.h: Wrappers around <rx> and the SRELL regex library. -------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday December 9, 2022
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library provides an abstraction wrapper around `<rx>`: the `Expression`
 * class representing a compiled regular experession and its operations, and the
 * `Capture` class representing a regular expression match and its capture
 * groups.
 *
 * The following free-functions are defined:
 *
 * - `rx::def(s, flags=ECMAScript)`: Compiles a regular expression, using
 *   ECMAScript syntax by default.
 * - `rx::idef`: Same as `rx::def`, but also with the `icase` flag by default
 *   making the regular expression captures case-insensitive.
 * - `rx::match(expr, s)`: Returns `true` if the given string `s` is matched by
 *   the regular expression, `false` otherwise.
 * - `rx::capture(expr, s)`: Matches the given string `s` and return a `Capture`
 *   object, which contains any capture groups matched.
 * - `rx::replace(expr, s, fmt)`: Replaces occurrences of the regex `expr` in
 *   `s` with a string based on the `std::regex_replace` format `fmt`, which
 *   varies depending on the regex format (ECMAScript by default).
 */

#ifndef __MOONLIGHT_RX_H
#define __MOONLIGHT_RX_H

#include <vector>
#include <string>
#include "moonlight/string.h"
#include "moonlight/collect.h"
#include "SRELL/srell.hpp"

namespace moonlight {
namespace rx {

typedef srell::regex Expression;

inline Expression def(const std::string& rx_str) {
    return Expression(rx_str, srell::regex_constants::ECMAScript);
}

inline Expression idef(const std::string& rx_str) {
    return Expression(rx_str, srell::regex_constants::ECMAScript | srell::regex_constants::icase);
}

inline bool match(const Expression& rx, const std::string& str) {
    return srell::regex_search(str.begin(), str.end(), rx);
}

// ------------------------------------------------------------------
class Capture {
 public:
     explicit Capture(const srell::smatch& smatch)
     : _length(smatch.length()), _groups(smatch.begin(), smatch.end()) { }

     Capture()
     : _length(0) { }

     unsigned int length() const {
         return _length;
     }

     const std::string& group(size_t offset = 0) const {
         return _groups[offset];
     }

     const std::string str() const {
         return group();
     }

     const std::vector<std::string>& groups() const {
         return _groups;
     }

     operator bool() const {
         return _groups.size() > 0;
     }

     friend std::ostream& operator<<(std::ostream& out, const Capture& capture) {
         auto literal_matches = collect::map<std::string>(capture.groups(), str::literalize);
         out << "Capture<" << str::join(literal_matches, ",") << ">";
         return out;
     }

 private:
     unsigned int _length;
     std::vector<std::string> _groups;
};

template<class BiIter>
Capture capture(const Expression& rx, const BiIter begin, const BiIter end) {
    srell::smatch smatch;
    if (srell::regex_search(begin, end, smatch, rx)) {
        return Capture(smatch);
    }
    return Capture();
}

inline Capture capture(const Expression& rx, const std::string& str) {
    return capture(rx, str.begin(), str.end());
}

inline std::string replace(const Expression& rx, const std::string& src, const std::string& format) {
    std::string result;
    srell::regex_replace(std::back_inserter(result), src.begin(), src.end(), rx, format);
    return result;
}

}  // namespace rx
}  // namespace moonlight

#endif /* !__MOONLIGHT_RX_H */
