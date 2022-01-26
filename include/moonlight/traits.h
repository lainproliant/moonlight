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
constexpr std::string_view wrapped_type_name()
{
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

} // namespace _type_name

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

}

#endif /* !__MOONLIGHT_TRAITS_H */
