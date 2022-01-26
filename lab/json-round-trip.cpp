/*
 * json_round_trip.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday April 12, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/json.h"
#include <iostream>

using namespace moonlight;

int main() {
    auto obj = json::read<json::Object>(std::cin);
    json::write(std::cout, obj);
    return 0;
}
