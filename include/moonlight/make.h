/*
 * ## make.h: A simple wrapper around `std::make_shared`. -----------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_MAKE_H
#define __MOONLIGHT_MAKE_H

#include <memory>
#include <utility>

//-------------------------------------------------------------------
template<typename T, typename... TD>
std::shared_ptr<T> make(TD&&... params) {
    return std::make_shared<T>(std::forward<TD>(params)...);
}

#endif /* !__MOONLIGHT_MAKE_H */
