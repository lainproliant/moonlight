/*
 * uuid.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday June 23, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "sole.hpp"
#include <iostream>

int main() {
    auto uuid = sole::uuid4();
    std::cout << uuid.pretty() << std::endl;
    std::cout << uuid.str() << std::endl;
    std::cout << uuid.base62() << std::endl;
    std::cout << std::hex << uuid.ab << " " << uuid.cd << std::endl;
    return 0;
}
