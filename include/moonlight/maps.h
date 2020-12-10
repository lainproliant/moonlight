/*
 * map.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_MAPS_H
#define __MOONLIGHT_MAPS_H

#include <map>
#include "moonlight/exceptions.h"

namespace moonlight {
namespace maps {

/**------------------------------------------------------------------
 * Copy a vector of the keys from the given map-like iterable.
 */
template<typename T>
inline std::vector<typename T::key_type> keys(const T& map) {
   std::vector<typename T::key_type> kvec;

   for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
      kvec.push_back(iter->first);
   }

   return kvec;
}

/**------------------------------------------------------------------
 * Copy a vector of the values from the given map-like iterable.
 */
template<typename T>
inline std::vector<typename T::mapped_type> values(const T& map) {
   std::vector<typename T::mapped_type> vvec;

   for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
      vvec.push_back(iter->second);
   }

   return vvec;
}

template<typename T>
inline const typename T::mapped_type& const_value(const T& map, const typename T::key_type& key) {
   auto iter = map.find(key);
   if (iter != map.end()) {
      return iter->second;
   } else {
      throw core::IndexException("Key does not exist in map.");
   }
}

template<typename T>
inline const typename T::mapped_type& get(const T& map, const typename T::key_type& key, const typename T::mapped_type& default_value) {
    auto iter = map.find(key);
    if (iter == map.end()) {
        return default_value;
    } else {
        return iter->second;
    }
}

template<typename T>
inline std::map<typename T::mapped_type, typename T::key_type> invert(const T& map) {
    std::map<typename T::mapped_type, typename T::key_type> inverse;
    for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
        map.insert({iter->second, iter->first});
    }
    return inverse;
}

}
}

#endif /* !__MOONLIGHT_MAPS_H */
