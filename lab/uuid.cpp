/*
 * uuid.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday June 23, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/uuid.h"
#include <iostream>

int main() {
    auto gen = moonlight::uuid::Generator();
    auto uuid = gen();
    std::cout << uuid << std::endl;
    return 0;
}
