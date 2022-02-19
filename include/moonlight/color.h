/*
 * color.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday March 14, 2021
 *
 * HSV/RGB conversion algorithms adapted from <http://dystopiancode.blogspot.com/2012/06/hsv-rgb-conversion-algorithms-in-c.html>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_COLOR_H
#define __MOONLIGHT_COLOR_H

#include <cmath>
#include <ostream>
#include <regex>
#include <inttypes.h>
#include "moonlight/exceptions.h"

namespace moonlight {
namespace color {

/** -----------------------------------------------------------------
 */
template<class T>
bool in_range(T start, T end, T value) {
    return value >= start && value <= end;
}

/** -----------------------------------------------------------------
 */
struct fRGB;
struct uRGB;

struct fHSV {
    float h;
    float s;
    float v;

    explicit operator fRGB() const;
    explicit operator uRGB() const;

    friend std::ostream& operator<<(std::ostream& out, const fHSV& hsv) {
        std::ios out_state(nullptr);
        out_state.copyfmt(out);
        out << "fHSV<" << std::fixed << std::setprecision(2) <<
            hsv.h << ", " << hsv.s << ", " << hsv.v << ">";
        out.copyfmt(out_state);
        return out;
    }

    fHSV normalize() const {
        float hx = fmodf(h, 360.0);
        if (hx < 0.0) {
            hx += 360.0;
        }

        return {
            hx,
            fmaxf(fminf(1.0, s), 0.0),
            fmaxf(fminf(1.0, v), 0.0),
        };
    }
};

/** -----------------------------------------------------------------
 */
struct uRGB {
    unsigned char r;
    unsigned char g;
    unsigned char b;

    static uRGB of(unsigned int c) {
        return {
            (unsigned char)((c >> 16) & 0xFF),
            (unsigned char)((c >> 8) & 0xFF),
            (unsigned char)(c & 0xFF)
        };
    }

    static uRGB of(const std::string& s) {
        std::string sx;
        if (s.starts_with("#")) {
            sx = s.substr(1);
        } else {
            sx = s;
        }

        return of(std::stoul(sx, nullptr, 16));
    }

    static bool is_valid(const std::string& s) {
        static const std::regex rx("#?[0-9a-fA-F]{6}");
        return std::regex_match(s, rx);
    }

    static uRGB validate(const std::string& s) {
        if (! is_valid(s)) {
            THROW(core::ValueError, "RGB color string is not valid: " + s);
        }
        return of(s);
    }

    explicit operator fRGB() const;
    explicit operator fHSV() const;

    explicit operator unsigned int() const {
        return (r << 16) | (g << 8) | b;
    }

    bool operator==(const uRGB& rhs) const {
        return r == rhs.r && g == rhs.g && b == rhs.b;
    }

    std::string str() const {
        char buf[16];
        snprintf(buf, 16u, "#%06" PRIX8, static_cast<unsigned int>(*this));
        return std::string(buf);
    }

    friend std::ostream& operator<<(std::ostream& out, const uRGB& rgb) {
        std::ios out_state(nullptr);
        out_state.copyfmt(out);
        out << "uRGB<" << std::hex << rgb.str() << ">";
        out.copyfmt(out_state);
        return out;
    }
};

/** -----------------------------------------------------------------
 */
struct fRGB {
    float r;
    float g;
    float b;

    explicit operator uRGB() const;
    explicit operator fHSV() const;

    friend std::ostream& operator<<(std::ostream& out, const fRGB& rgb) {
        std::ios out_state(nullptr);
        out_state.copyfmt(out);
        out << "fRGB<" << std::fixed << std::setprecision(2) <<
            rgb.r << ", " << rgb.g << ", " << rgb.b << ">";
        out.copyfmt(out_state);
        return out;
    }

    fRGB normalize() const {
        return {
            fmaxf(fminf(1.0, r), 0.0),
            fmaxf(fminf(1.0, g), 0.0),
            fmaxf(fminf(1.0, b), 0.0)
        };
    }
};

/** -----------------------------------------------------------------
 */
inline fHSV::operator uRGB() const {
    return static_cast<uRGB>(static_cast<fRGB>(*this));
}

/** -----------------------------------------------------------------
 */
inline fHSV::operator fRGB() const {
    fRGB rgb = { 0.0f, 0.0f, 0.0f };
    float c = 0.0f, m = 0.0f, x = 0.0f;
    float hx = fmodf(h, 360.0f);
    c = v * s;
    x = c * (1.0f - fabsf(fmodf(hx / 60.0f, 2) - 1.0f));
    m = v - c;
    if (hx >= 0.0 && hx < 60.0) {
        rgb = { c + m, x + m, m };
    } else if (hx >= 60.0f && hx < 120.0f) {
        rgb = { x + m, c + m, m };
    } else if (hx >= 120.0f && hx < 180.0f) {
        rgb = { m, c + m, x + m };
    } else if (hx >= 180.f && hx < 240.0f) {
        rgb = { m, x + m, c + m };
    } else if (hx >= 240.0f && hx < 300.0f) {
        rgb = { x + m, m, c + m };
    } else if (hx >= 300.0 && hx < 360.0) {
        rgb = { c + m, m, x + m };
    } else {
        rgb = { m, m, m };
    }

    return rgb;
}

/** -----------------------------------------------------------------
 */
inline uRGB::operator fRGB() const {
    return {
        .r = r / 255.0f,
        .g = g / 255.0f,
        .b = b / 255.0f
    };
}

/** -----------------------------------------------------------------
 */
inline uRGB::operator fHSV() const {
    return static_cast<fHSV>(static_cast<fRGB>(*this));
}

/** -----------------------------------------------------------------
 */
inline fRGB::operator uRGB() const {
    return {
        .r = static_cast<unsigned char>(::floor(r * 255.0f)),
        .g = static_cast<unsigned char>(::floor(g * 255.0f)),
        .b = static_cast<unsigned char>(::floor(b * 255.0f))
    };
}

/** -----------------------------------------------------------------
 */
inline fRGB::operator fHSV() const {
    fHSV hsv = { 0.0f, 0.0f, 0.0f };
    float M = ::fmaxf(r, ::fmaxf(g, b));
    float m = ::fminf(r, ::fminf(g, b));
    float c = M - m;
    hsv.v = M;

    if (c != 0.0f) {
        if (M == r) {
            hsv.h = ::fmodf(((g - b) / c), 6.0);
        } else if (M == g) {
            hsv.h = (b - r) / c + 2.0f;
        } else /* if M == b */ {
            hsv.h = (r - g) / c + 4.0f;
        }
        hsv.h *= 60.0f;
        if (hsv.h < 0.0) {
            hsv.h += 360.0f;
        }
        hsv.s = c / hsv.v;
    }

    return hsv;
}

}
}

#endif /* !__MOONLIGHT_COLOR_H */
