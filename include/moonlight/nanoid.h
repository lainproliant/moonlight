/*
 * ## nanoid.h: a simple C++ implementation of NanoID. --------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday February 19, 2022
 *
 * Distributed under terms of the MIT license.
 *
 * ## Credits -------------------------------------------------------
 * This library is based on the following efforts:
 * - The C header-only NanoID library by Akshay (nerdypepper)
 * 		- https://github.com/NerdyPepper/nanoid
 * - The more robust C++ nanoid_cpp library by Mykola Morozov
 *      - https://github.com/mcmikecreations/nanoid_cpp
 *
 * ## Usage ---------------------------------------------------------
 * This library exposes a single template class, `nanoid::IDFactory`.  This
 * class generates a small uniformly-random unique ID as an `std::string` via
 * its `generate()` method.
 *
 * By default, this ID will be generated using `std::random_device`,
 * `std::uniform_int_distribution<int>`, and are 21 characters long.  The
 * following alternative pre-built alphabets are also provided:
 *
 * - `DEFAULT_ALPHABET`: All alpha-numerics plus `-` and `_`.
 * - `NUMBERS`
 * - `UPPERCASE`
 * - `LOWERCASE`
 * - `ALPHANUMERIC`
 * - `NO_LOOK_ALIKES`: An abridged alphabet with no look alike characters.
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

template<class RF = std::random_device>
class IDFactory {
public:
    IDFactory() : rd(), rng(rd()) { }

    std::string generate(size_t size = DEFAULT_SIZE, const std::string& alphabet = DEFAULT_ALPHABET) {
        std::string id;
        std::uniform_int_distribution<int> dist(0, alphabet.size() - 1);

        while (id.size() < size) {
            id.push_back(alphabet[dist(rng)]);
        }

        return id;
    }

private:
    RF rd;
    std::mt19937 rng;
};

}  // namespace nanoid
}  // namespace moonlight


#endif /* !__MOONLIGHT_NANOID_H */
