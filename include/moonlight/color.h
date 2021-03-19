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
#include "tinyformat/tinyformat.h"

namespace moonlight {
namespace color {

//-------------------------------------------------------------------
template<class T>
bool in_range(T start, T end, T value) {
    return value >= start && value <= end;
}

//-------------------------------------------------------------------
struct fRGB;
struct uRGB;

struct fHSV {
    float h;
    float s;
    float v;

    explicit operator fRGB() const;
    explicit operator uRGB() const;

    friend std::ostream& operator<<(std::ostream& out, const fHSV& hsv) {
        tfm::format(out, "fHSV<%0.2f°, %0.2f, %0.2f>", hsv.h, hsv.s, hsv.v);
        return out;
    }
};

//-------------------------------------------------------------------
struct uRGB {
    unsigned int r;
    unsigned int g;
    unsigned int b;

    static uRGB of(unsigned int c) {
        return {
            (c >> 16) & 0xFF,
            (c >> 8) & 0xFF,
            c & 0xFF
        };
    }

    explicit operator fRGB() const;
    explicit operator fHSV() const;

    explicit operator unsigned int() const {
        return (r << 16) | (g << 8) | b;
    }

    bool operator==(const uRGB& rhs) const {
        return r == rhs.r && g == rhs.g && b == rhs.b;
    }

    friend std::ostream& operator<<(std::ostream& out, const uRGB& rgb) {
        tfm::format(out, "uRGB<%06X>", static_cast<unsigned int>(rgb));
        return out;
    }
};

//-------------------------------------------------------------------
struct fRGB {
    float r;
    float g;
    float b;

    explicit operator uRGB() const;
    explicit operator fHSV() const;

    friend std::ostream& operator<<(std::ostream& out, const fRGB& rgb) {
        tfm::format(out, "fRGB<%0.2f, %0.2f, %0.2f>", rgb.r, rgb.g, rgb.b);
        return out;
    }
};

//-------------------------------------------------------------------
inline fHSV::operator uRGB() const {
    return static_cast<uRGB>(static_cast<fRGB>(*this));
}

//-------------------------------------------------------------------
inline fHSV::operator fRGB() const {
    fRGB rgb = { 0.0f, 0.0f, 0.0f };
    float c = 0.0f, m = 0.0f, x = 0.0f;
    c = v * s;
    x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2) - 1.0f));
    m = v - c;
    if (h >= 0.0 && h < 60.0) {
        rgb = { c + m, x + m, m };
    } else if (h >= 60.0f && h < 120.0f) {
        rgb = { x + m, c + m, m };
    } else if (h >= 120.0f && h < 180.0f) {
        rgb = { m, c + m, x + m };
    } else if (h >= 180.f && h < 240.0f) {
        rgb = { m, x + m, c + m };
    } else if (h >= 240.0f && h < 300.0f) {
        rgb = { x + m, m, c + m };
    } else if (h >= 300.0 && h < 360.0) {
        rgb = { c + m, m, x + m };
    } else {
        rgb = { m, m, m };
    }

    return rgb;
}

//-------------------------------------------------------------------
inline uRGB::operator fRGB() const {
    return {
        .r = r / 255.0f,
        .g = g / 255.0f,
        .b = b / 255.0f
    };
}

//-------------------------------------------------------------------
inline uRGB::operator fHSV() const {
    return static_cast<fHSV>(static_cast<fRGB>(*this));
}

//-------------------------------------------------------------------
inline fRGB::operator uRGB() const {
    return {
        .r = static_cast<unsigned int>(::floor(r * 255.0f)),
        .g = static_cast<unsigned int>(::floor(g * 255.0f)),
        .b = static_cast<unsigned int>(::floor(b * 255.0f))
    };
}

//-------------------------------------------------------------------
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
