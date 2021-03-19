#include "moonlight/ansi.h"
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

    for (int x = 0; x < 360; x++) {
        color::fHSV hsv = { static_cast<float>(x), 1.0f, 1.0f };
        auto rgb = static_cast<color::uRGB>(hsv);
        std::cout << bg::rgb(rgb)(" ");
    }

    std::cout << std::endl;

    return 0;
}
