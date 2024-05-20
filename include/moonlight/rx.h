/*
 * rx.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday December 9, 2022
 *
 * Distributed under terms of the MIT license.
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
