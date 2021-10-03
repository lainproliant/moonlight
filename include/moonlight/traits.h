/*
 * traits.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday September 24, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_TRAITS_H
#define __MOONLIGHT_TRAITS_H

#include <type_traits>
#include <iterator>
#include <memory>

namespace moonlight {

template<class T, class = void>
struct is_iterable_type : public std::false_type {
    typedef std::void_t<> type;
};

template<class T>
struct is_iterable_type<T, std::void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>()))>> : public std::true_type {

    typedef T type;
};

template<class T, class = void>
struct is_map_type : public std::false_type {
    typedef std::void_t<> type;
};

template<class T>
struct is_map_type<T, std::void_t<
    typename T::key_type,
    typename T::mapped_type,
    decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>> : public std::true_type {
    typedef T type;
};

template<class T>
struct is_shared_ptr : public std::false_type {
    typedef std::void_t<> type;
};

template<class T>
struct is_shared_ptr<std::shared_ptr<T>> : public std::true_type {
    typedef std::shared_ptr<T> type;
};

template<class T>
struct is_raw_pointer : public std::false_type {
    typedef std::void_t<> type;
};

template<class T>
struct is_raw_pointer<T*> : public std::true_type {
    typedef T* type;
};

}


#endif /* !__MOONLIGHT_TRAITS_H */
