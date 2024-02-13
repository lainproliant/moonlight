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

#include <regex>

namespace moonlight {
namespace rx {

inline std::regex def(const std::string& rx_str) {
    return std::regex(rx_str, std::regex_constants::ECMAScript);
}

inline bool match(const std::regex& rx, const std::string& str) {
    return std::regex_search(str.begin(), str.end(), rx);
}

// ------------------------------------------------------------------
class Capture {
public:
    Capture(const std::smatch& smatch)
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

private:
    unsigned int _length;
    const std::vector<std::string> _groups;
};

inline Capture capture(const std::regex& rx, const std::string& str) {
    std::smatch smatch = {};
    if (std::regex_search(str.begin(), str.end(), smatch, rx)) {
        return Capture(smatch);
    }
    return Capture();
}

}
}

#endif /* !__MOONLIGHT_RX_H */
