/*
 * ## slice.h -------------------------------------------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library mimics the functionality of Python's array slicing in C++.
 * It offers the following function templates:
 *
 * - `slice(C, start=None, end=None)`: Slices the given collection.
 * - `slice_offset(C, offset, clip=false)`: Gets the offset into the iterable
 *   collection represented by the given offset value.
 */

#ifndef __MOONLIGHT_SLICE_H
#define __MOONLIGHT_SLICE_H

#include <optional>

#include "moonlight/exceptions.h"

namespace moonlight {

//-------------------------------------------------------------------
template<class C>
inline size_t slice_offset(const C& coll, int offset, bool clip = false) {
    if (offset < 0) {
        offset += coll.size();
        if (offset < 0) {
            if (clip) {
                offset = 0;
            } else {
                THROW(core::IndexError, "Index out of range (-).");
            }
        }
    }

    if (offset >= coll.size()) {
        if (clip) {
            offset = coll.size();
        } else {
            THROW(core::IndexError, "Index out of range (+).");
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

}  // namespace moonlight


#endif /* !__MOONLIGHT_SLICE_H */
