/*
 * nanoid.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday February 20, 2022
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/nanoid.h"
#include "moonlight/cli.h"
#include "moonlight/file.h"
#include "tinyformat/tinyformat.h"

const std::string CHARACTER_SPACE = moonlight::nanoid::NUMBERS + moonlight::nanoid::LOWERCASE;

int main() {
    auto infile = moonlight::file::BufferedInput(std::cin);
    while (! infile.is_exhausted()) {
        auto line = infile.getline();
        if (line.size() > 0) {
            std::cout << moonlight::nanoid::generate(8, CHARACTER_SPACE) << " " << line;
        }
    }
    return 0;
}
