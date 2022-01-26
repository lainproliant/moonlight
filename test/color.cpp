/*
 * color.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday March 18, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/color.h"
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::color;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight color tests")
    .test("verify HSV/RGB conversion and back", []() {
        auto rgbMagenta = uRGB::of(0xFF00FF);
        auto frgbMagenta = static_cast<fRGB>(rgbMagenta);
        auto hsvMagenta = static_cast<fHSV>(rgbMagenta);
        std::cout << "rgbMagenta = " << rgbMagenta << std::endl;
        std::cout << "frgbMagenta = " << frgbMagenta << std::endl;
        std::cout << "hsvMagenta = " << hsvMagenta << std::endl;
        ASSERT_EP_EQUAL(hsvMagenta.h, 300.0f, 0.01f);
        ASSERT_EP_EQUAL(hsvMagenta.s, 1.0f, 0.01f);
        ASSERT_EP_EQUAL(hsvMagenta.v, 1.0f, 0.01f);
        ASSERT_EQUAL(rgbMagenta, static_cast<uRGB>(hsvMagenta));
    })
    .test("verify HSV color wheel rotation and conversion back to RGB", []() {
        auto hsvRed = static_cast<fHSV>(uRGB::of(0xAC0000));
        auto rgbCyan = uRGB::of(0x00ACAC);
        fHSV hsvCyan = { hsvRed.h + 180.0f, hsvRed.s, hsvRed.v };
        std::cout << "static_cast<uRGB>(hsvCyan) = " << static_cast<uRGB>(hsvCyan) << std::endl;
        ASSERT_EQUAL(rgbCyan, static_cast<uRGB>(hsvCyan));
    })
    .run();
}
