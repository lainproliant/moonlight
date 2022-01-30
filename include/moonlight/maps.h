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
gen::Stream <typename M::key_type> keys(const M& map) {
    auto iter = map.begin();

    return gen::Stream<typename M::key_type>(
        [iter, &map]() mutable -> std::optional<typename M::key_type> {
            if (iter != map.end()) {
                return (iter++)->first;
            } else {
                return {};
            }
        }
    );
}

//-------------------------------------------------------------------
template<class M>
gen::Stream<typename M::mapped_type> values(const M& map) {
    auto iter = map.begin();

    return gen::Stream<typename M::mapped_type>(
        [iter, &map]() mutable -> std::optional<typename M::mapped_type> {
            if (iter != map.end()) {
                return (iter++)->second;
            } else {
                return {};
            }
        }
    );
}

}
}

#endif /* !__MOONLIGHT_MAPS_H */
