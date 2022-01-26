/*
 * exceptions.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday January 21, 2022
 *
 * Distributed under terms of the MIT license.
 */

#include <iostream>
#include "moonlight/exceptions.h"

int main() {
    try {
        throw moonlight::core::Exception("test!", LOCATION);

    } catch (const moonlight::core::Exception& e) {
        try {
            throw moonlight::core::AssertionFailure("oops", LOCATION).caused_by(e);

        } catch (const moonlight::core::Exception& e2) {
            std::cout << e2 << std::endl;
        }
    }

    auto stacktrace = moonlight::debug::StackTrace::generate(LOCATION);
    std::cout << stacktrace << std::endl;

    return 0;
}
