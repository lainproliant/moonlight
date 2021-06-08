/*
 * shlex-unsafe.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 8, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/shlex.h"
#include "moonlight/ansi.h"
#include <iostream>

using namespace moonlight;

int main() {

    std::string line;

    for (;;) {
        std::cout << fg::magenta("> ");
        if (! std::getline(std::cin, line)) {
            break;
        }

        std::cout << "\"" << shlex::ShellLexer::quote(line) << "\"" << std::endl;
    }

    return 0;
}
