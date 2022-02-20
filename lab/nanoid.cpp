/*
 * nanoid.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday February 20, 2022
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/nanoid.h"
#include <iostream>

int main() {
    for (int x = 0; x < 100; x++) {
        std::cout << moonlight::nanoid::generate(x) << std::endl;
    }
    return 0;
}
