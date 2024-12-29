/*
 * ## map.h: Template methods for operating on maps. ----------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library contains some useful template methods for operating on maps.
 *
 * - `map::invert(m)`: Creates a new map where the keys and values from the
 *   given map are inverted.  This may result in a smaller map if the values are
 *   not unique, and only the latest value-as-key in the sequence will be
 *   present in the inverted map.
 * - `map::items(m)`: Creates a `gen::Stream<std::pair<K, V>>` of the key-value
 *   pairs from the map, allowing for further functional composition across the
 *   items in the map.
 * - `map::keys(m)`: Creates a `gen::Stream<K>` of the keys from the map.
 * - `map::values(m)`: Creates a `gen::Stream<V>` of the values from the map.
 */

#ifndef __MOONLIGHT_MAPS_H
#define __MOONLIGHT_MAPS_H

#include <map>
#include <utility>

#include "moonlight/generator.h"

namespace moonlight {
namespace maps {

//-------------------------------------------------------------------
template<class M>
inline std::map<class M::mapped_type, class M::key_type> invert(const M& map) {
    std::map<class M::mapped_type, class M::key_type> inverse;
    for (auto iter = map.cbegin(); iter != map.cend(); iter++) {
        map.insert({iter->second, iter->first});
    }
    return inverse;
}

//-------------------------------------------------------------------
template<class M>
gen::Stream <std::pair<typename M::key_type, typename M::mapped_type>> items(const M& map) {
    auto iter = map.begin();

    return gen::Stream<std::pair<typename M::key_type, typename M::mapped_type>>(
        [iter, &map]() mutable -> std::optional<std::pair<typename M::key_type, typename M::mapped_type>> {
            if (iter != map.end()) {
                auto& val = *iter;
                iter++;
                return val;
            } else {
                return {};
            }
        });
}

//-------------------------------------------------------------------
template<class M>
gen::Stream <typename M::key_type> keys(const M& map) {
    return maps::items(map).template transform<typename M::key_type>([](const auto& item) {
        return item.first;
    });
}

//-------------------------------------------------------------------
template<class M>
gen::Stream<typename M::mapped_type> values(const M& map) {
    return maps::items(map).template transform<typename M::mapped_type>([](const auto& item) {
        return item.second;
    });
}

}  // namespace maps
}  // namespace moonlight

#endif /* !__MOONLIGHT_MAPS_H */
