/*
 * ## traits.h: Various SFINAE traits for compile-time checks. -------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday September 24, 2021
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ----------------------------------------------------------
 * This header contains various template functions which can be used in
 * constexpr contexts in compile time to determine various attributes about
 * types.  These functions take advantage of the C++ concept known as SFINAE
 * (Substitution Failure Is Not An Error), which deduces properties of types
 * based on the template parameter substitution behavior of the compiler.
 *
 * The following template traits are defined:
 *
 * - `is_streamable<T>()`: Determine if the given type supports being streamed
 *   to an `std::ostream` output stream via `operator<<`.
 * - `is_iterable_type<T>()`: Determine if the given type supports forward
 *   iteration via `std::begin()` and `std::end()`.  If true, that means that
 *   this type can be iterated via standard `<algorithm>` style templates and
 *   the ranged-for loop.
 * - `is_map_type<T>()`: Determine if the given type is an `std::map`-like
 *   container that defines a `T::key_type`, a `T::mapped_type`, and an
 *   `operator[]` which accepts the `T::key_type` and returns a reference to
 *   `T::mapped_type`.
 * - `is_shared_ptr<T>()`: Determine if the given type is based on
 *   `std::shared_ptr`.
 * - `is_raw_pointer<T>()`: Determine if the given type is a raw pointer.
 * - `type_name<T>()`: A `constexpr` that yields the given type's name as an
 *   `std::string`.
 * - `type_name_view<T>()`: A `constexpr` that yields an `std::string_view` into
 *   the given type's name.
 */

#ifndef __MOONLIGHT_TRAITS_H
#define __MOONLIGHT_TRAITS_H

#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <iostream>

namespace moonlight {

//-------------------------------------------------------------------
template<class, typename=void>
struct is_streamable : std::false_type {};

//-------------------------------------------------------------------
template<class T>
struct is_streamable<T, std::void_t<
decltype(operator<<(std::declval<std::ostream&>, std::declval<const T&>))>>
: public std::true_type {

    typedef T type;
};

//-------------------------------------------------------------------
template<class T, class = void>
struct is_iterable_type : public std::false_type {
    typedef std::void_t<> type;
};


//-------------------------------------------------------------------
template<class T>
struct is_iterable_type<T, std::void_t<
decltype(std::begin(std::declval<T>())),
decltype(std::end(std::declval<T>()))>> : public std::true_type {

    typedef T type;
};

//-------------------------------------------------------------------
template<class T, class = void>
struct is_map_type : public std::false_type {
    typedef std::void_t<> type;
};

//-------------------------------------------------------------------
template<class T>
struct is_map_type<T, std::void_t<
typename T::key_type,
         typename T::mapped_type,
         decltype(std::declval<T&>()[std::declval<const typename T::key_type&>()])>> : public std::true_type {
             typedef T type;
         };

//-------------------------------------------------------------------
template<class T>
struct is_shared_ptr : public std::false_type {
    typedef std::void_t<> type;
};

//-------------------------------------------------------------------
template<class T>
struct is_shared_ptr<std::shared_ptr<T>> : public std::true_type {
    typedef std::shared_ptr<T> type;
};

//-------------------------------------------------------------------
template<class T>
struct is_raw_pointer : public std::false_type {
    typedef std::void_t<> type;
};

//-------------------------------------------------------------------
template<class T>
struct is_raw_pointer<T*> : public std::true_type {
    typedef T* type;
};

//-------------------------------------------------------------------
namespace _type_name {

template<class T> constexpr std::string_view type_name_view();

template<>
constexpr std::string_view type_name_view<void>()
{ return "void"; }


using type_name_prober = void;

    template<class T>
constexpr std::string_view wrapped_type_name() {
#ifdef __clang__
    return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

constexpr std::size_t wrapped_type_name_prefix_length() {
    return wrapped_type_name<type_name_prober>().find(type_name_view<type_name_prober>());
}

constexpr std::size_t wrapped_type_name_suffix_length() {
    return wrapped_type_name<type_name_prober>().length()
    - wrapped_type_name_prefix_length()
    - type_name_view<type_name_prober>().length();
}

}  // namespace _type_name

//-------------------------------------------------------------------
template<class T>
constexpr std::string_view type_name_view() {
    constexpr auto wrapped_name = _type_name::wrapped_type_name<T>();
    constexpr auto prefix_length = _type_name::wrapped_type_name_prefix_length();
    constexpr auto suffix_length = _type_name::wrapped_type_name_suffix_length();
    constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;
    return wrapped_name.substr(prefix_length, type_name_length);
}

/**
 * type_name(T)
 *
 * Based on an answer from StackOverflow: https://stackoverflow.com/a/56600402
 * Credits to @HowardHinnant, @康桓瑋, @Val, and @einpoklum.
 */
template<class T>
std::string type_name() {
    return std::string(type_name_view<T>());
}

}  // namespace moonlight

#endif /* !__MOONLIGHT_TRAITS_H */
