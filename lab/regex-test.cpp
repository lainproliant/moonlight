/*
 * regex-test.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday March 9, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/lex.h"
#include "moonlight/ansi.h"
#include <iostream>

using namespace moonlight;

void repl() {
    std::string rx_str, line;
    std::smatch smatch = {};
    std::cout << fg::magenta("line> ");
    std::cin >> line;
    std::cout << fg::cyan("rx> ");
    std::cin >> rx_str;

    std::regex rx(rx_str);
    if (std::regex_search(line.cbegin(), line.cend(), smatch, rx)) {
        std::cout << fg::bright(fg::green("[ match ]")) << std::endl;
    } else {
        std::cout << fg::bright(fg::red("[ no match ]")) << std::endl;
    }
}

int main() {
    for(;;) repl();
}
