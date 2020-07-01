/*
 * slice.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_SLICE_H
#define __MOONLIGHT_SLICE_H

#include "moonlight/exceptions.h"
#include <optional>

namespace moonlight {

//-------------------------------------------------------------------
class SliceError : public moonlight::core::Exception {
    using Exception::Exception;
};

//-------------------------------------------------------------------
template<class C>
inline size_t slice_offset(const C& coll, int offset, bool clip = false) {
    if (offset < 0) {
        offset += coll.size();
        if (offset < 0) {
            if (clip) {
                offset = 0;
            } else {
                throw SliceError("Index out of range (-).");
            }
        }
    }

    if (offset >= coll.size()) {
        if (clip) {
            offset = coll.size();
        } else {
            throw SliceError("Index out of range (+).");
        }
    }

    return offset;
}

//-------------------------------------------------------------------
template<class C>
inline typename C::value_type slice(const C& coll, int int_offset) {
    size_t offset = slice_offset(coll, int_offset);
    return coll[offset];
}

//-------------------------------------------------------------------
template<class C>
inline C slice(const C& coll, std::optional<int> int_offset_start, std::optional<int> int_offset_end) {
    C result;
    size_t offset_start = slice_offset(coll, int_offset_start.value_or(0), true);
    size_t offset_end = slice_offset(coll, int_offset_end.value_or(coll.size()), true);

    for (size_t x = offset_start; x < offset_end; x++) {
        result.push_back(coll[x]);
    }

    return result;
}

}


#endif /* !__MOONLIGHT_SLICE_H */
