/*
 * nanoid.h
 *
 * A simple C++ implementation of NanoID.
 *
 * Based on:
 *
 * - The C header-only NanoID library by Akshay (nerdypepper)
 * 		- https://github.com/NerdyPepper/nanoid
 * - The more robust C++ nanoid_cpp library by Mykola Morozov
 *      - https://github.com/mcmikecreations/nanoid_cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday February 19, 2022
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_NANOID_H
#define __MOONLIGHT_NANOID_H

#include <random>
#include <string>

namespace moonlight {
namespace nanoid {

const std::string DEFAULT_ALPHABET = "_-0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string NUMBERS = "0123456789";

const std::string UPPERCASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

const std::string LOWERCASE = "abcdefghijklmnopqrstuvwxyz";

const std::string ALPHANUMERIC = NUMBERS + LOWERCASE + UPPERCASE;
const std::string NO_LOOK_ALIKES = "2346789ABCDEFGHJKLMNPQRTUVWXYZabcdefghijkmnpqrtwxyz";

const size_t DEFAULT_SIZE = 21;

template<class RandomFactory = std::random_device>
inline std::string generate(size_t size = DEFAULT_SIZE, const std::string& alphabet = DEFAULT_ALPHABET) {
    std::string id;

    RandomFactory rd;
    std::uniform_int_distribution<int> dist(0, alphabet.size() - 1);

    while (id.size() < size) {
        id.push_back(alphabet[dist(rd)]);
    }

    return id;
}

}
}


#endif /* !__MOONLIGHT_NANOID_H */
