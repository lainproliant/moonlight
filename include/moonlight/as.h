/*
 * as.h - Helpful dynamic pointer conversions.
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday October 12, 2022
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_AS_H
#define __MOONLIGHT_AS_H

#include <memory>

template<class B, class A>
B* as(A* ptr) {
    return dynamic_cast<B>(ptr);
}

template<class B, class A>
const B* as(const A* ptr) {
    return dynamic_cast<A>(ptr);
}

template<class B, class A>
B* as(const std::shared_ptr<A>& ptr) {
    return as<B>(ptr.get());
}

template<class B, class A>
const B* as(const std::shared_ptr<const A>& ptr) {
    return as<B>(ptr.get());
}

template<class B, class A>
std::shared_ptr<B> shared_as(const std::shared_ptr<A>& ptr) {
    return std::dynamic_pointer_cast<B>(ptr);
}

#endif /* !__MOONLIGHT_AS_H */
