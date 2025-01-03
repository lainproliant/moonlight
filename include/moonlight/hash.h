/*
 * ## hash.h: Hashing functions and utilities. ----------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library just contains `hash::combine`, lifted from `boost::hash_combine`,
 * a useful function for combining the result of a series of hashes over different
 * objects into a single hash value.
 */

#ifndef __MOONLIGHT_HASH_H
#define __MOONLIGHT_HASH_H

#include <functional>

namespace moonlight {
namespace hash {

/**
 * Lifted from boost::hash_combine.
 */
template<class T>
inline void combine(std::size_t& seed, const T& value) {
    std::hash<T> hash_function;
    seed ^= hash_function(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}  // namespace hash
}  // namespace moonlight


#endif /* !__MOONLIGHT_HASH_H */
