/*
 * meta.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday April 6, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_META_H
#define __MOONLIGHT_META_H

namespace moonlight {

// ------------------------------------------------------------------
// Assists with static assertions that can be used to limit template
// expansions to a list of possibilities.
//
// This is used to provide a "default invalid" implementation of a
// templatized function inside a class definition.
//
template< typename T >
struct always_false {
    enum { value = false };
};

}
#endif /* !__MOONLIGHT_META_H */
