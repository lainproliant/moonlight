/*
 * hsv.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday March 26, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/color.h"
#include "moonlight/cli.h"
#include "tinyformat.h"
#include <iostream>

using namespace moonlight;

struct hsv_adj {
    float h;
    float s;
    float v;
};

color::uRGB adjust_color(const color::uRGB& input, const hsv_adj& adj) {
    auto hsv = static_cast<color::fHSV>(input);
    hsv.h += adj.h;
    hsv.s += adj.s;
    hsv.v += adj.v;
    return static_cast<color::uRGB>(hsv.normalize());
}

int main(int argc, char** argv) {
    auto cmd = moonlight::cli::parse(
        argc, argv,
        { },
        { "hue", "h", "sat", "s", "val", "v" });

    std::vector<std::string> rgb_inputs;
    std::vector<color::uRGB> rgb_values;

    if (cmd.args().size() < 1) {
        std::string rgb_input;
        while (std::getline(std::cin, rgb_input)) {
            rgb_inputs.push_back(rgb_input);
        }
    } else {
        rgb_inputs = cmd.args();
    }

    if (rgb_inputs.size() < 1) {
        std::cerr << "At least one RGB value must be provided." << std::endl;
        return 1;
    }

    std::transform(rgb_inputs.begin(), rgb_inputs.end(), std::back_inserter(rgb_values),
                   [](const auto& s) { return color::uRGB::validate(s); });

    const hsv_adj hsv = {
        std::stof(cmd.get("h", "hue").value_or("0.0")),
        std::stof(cmd.get("s", "sat").value_or("0.0")),
        std::stof(cmd.get("v", "val").value_or("0.0")),
    };

    for (auto value : rgb_values) {
        if (hsv.h == 0.0 && hsv.s == 0.0 && hsv.v == 0.0) {
            auto hsv_value = static_cast<color::fHSV>(value);
            std::cout << hsv_value << std::endl;

        } else {
            auto adj_value = adjust_color(value, hsv);
            std::cout << adj_value.str() << std::endl;
        }

    }

    return 0;
}
