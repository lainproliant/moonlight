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

    bool valid() const {
        return (in_range(.0f, (float)M_2_PI, h),
                in_range(.0f, 1.0f, s),
                in_range(.0f, 1.0f, v));
    }

    explicit operator fRGB() const;
    explicit operator uRGB() const;
};

//-------------------------------------------------------------------
struct uRGB {
    unsigned int r;
    unsigned int g;
    unsigned int b;

    bool valid() const {
        return (in_range(0u, 255u, r) &&
                in_range(0u, 255u, g) &&
                in_range(0u, 255u, b));

    }

    explicit operator fRGB() const;
    explicit operator fHSV() const;
};

//-------------------------------------------------------------------
struct fRGB {
    float r;
    float g;
    float b;

    bool valid() const {
        return (in_range(0.0f, 1.0f, r) &&
                in_range(0.0f, 1.0f, g) &&
                in_range(0.0f, 1.0f, b));
    }

    explicit operator uRGB() const;
    explicit operator fHSV() const;
};

//-------------------------------------------------------------------
inline fHSV::operator uRGB() const {
    uRGB rgb;
    // TODO
    return rgb;
}

//-------------------------------------------------------------------
inline fHSV::operator fRGB() const {
    fRGB rgb;
    // TODO
    return rgb;
}

//-------------------------------------------------------------------
inline uRGB::operator fRGB() const {
    return {
        .r = r / 256.0f,
        .g = g / 256.0f,
        .b = b / 256.0f
    };
}

//-------------------------------------------------------------------
inline uRGB::operator fHSV() const {
    return static_cast<fHSV>(static_cast<fRGB>(*this));
}

//-------------------------------------------------------------------
inline fRGB::operator uRGB() const {
    return {
        .r = static_cast<unsigned int>(::floor(r * 256.0f)),
        .g = static_cast<unsigned int>(::floor(g * 256.0f)),
        .b = static_cast<unsigned int>(::floor(b * 256.0f))
    };
}

//-------------------------------------------------------------------
inline fRGB::operator fHSV() const {
    fHSV hsv = { .h = 0.0f, .s = 0.0f, .v = 0.0f };
    float M = ::fmaxf(r, ::fmaxf(g, b));
    float m = ::fminf(r, ::fminf(g, b));
    float c = M - m;

    if (c != 0.0f) {
        if (M == r) {
            hsv.h = ::fmodf(((g - b) / c), 6.0);
        } else if (M == g) {
            hsv.h = (b - r) / c + 2.0f;
        } else /* if M == b */ {
            hsv.h = (r - g) / c + 4.0f;
        }
        hsv.h *= 60;
        hsv.s = c / hsv.v;
    }

    return hsv;
}

}
}

#endif /* !__MOONLIGHT_COLOR_H */
