#include "moonlight/ansi.h"
#include "tinyformat/tinyformat.h"
#include <iostream>

using namespace moonlight;

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    const auto red = fg::bright(fg::red);
    const auto white = fg::bright(fg::white);
    const auto blue = fg::bright(fg::blue);
    std::cout << "Hello from the " << red("U") << white("S") << blue("A") << "!" << std::endl;
    std::cout << "This text is " << fg::reverse("reversed") << "." << std::endl;
    std::cout << "This text might " << fg::blink("blink") << " if your terminal supports it." << std::endl;

    for (int s = 0; s <= 4; s++) {
        if (s == 0) {
            tfm::printf("s=0.00        ");
            for (int v = 0; v < 60; v++) {
                color::fHSV hsv = { 0.0f, 0.0f, v / 60.0f };
                auto rgb = static_cast<color::uRGB>(hsv);
                std::cout << bg::rgb(rgb)(" ");
            }
            std::cout << std::endl;

        } else {
            for (int v = 1; v <= 4; v++) {
                tfm::printf("s=%0.2f,v=%0.2f ", s / 4.0f, v / 4.0f);
                for (int h = 0; h < 60; h++) {
                    color::fHSV hsv = { static_cast<float>(360.0f * (h / 60.0f)), s / 4.0f, v / 4.0f };
                    auto rgb = static_cast<color::uRGB>(hsv);
                    std::cout << bg::rgb(rgb)(" ");
                }
                std::cout << std::endl;
            }
        }
    }

    std::cout << std::endl;

    return 0;
}
