/*
 * variadic.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_VARIADIC_H
#define __MOONLIGHT_VARIADIC_H

#include <vector>

namespace moonlight {
namespace variadic {
/**------------------------------------------------------------------
 * An empty base structure used for iterating across variadic
 * template parameters in order.  Takes advantage of the fact
 * that initialization lists preserve order of parameters
 * when variadic template parameters are expanded in them.
 *
 * ```
 * template<typename T, typename... TV>
 * T sum(T init, const TV&&... values) {
 *    T total = init;
 *    variadic::pass{(total += values)...};
 * }
 * ```
 *
 * To use with void function calls or statements, provide
 * a second statement that yields a value, e.g. a const statement:
 *
 * ```
 * template<typename T, typename... TV>
 * void foreach(std::function<void(T)> f, const TV&&... values) {
 *    variadic::pass{(f(values), 0)...};
 * }
 * ```
 */
struct pass {
   template<typename... T> pass(T...) { }
};

//-------------------------------------------------------------------
template<typename T, typename... TD>
inline void _to_vector(std::vector<T>& vec) {
   (void)vec;
}

//-------------------------------------------------------------------
template<typename T, typename... TD>
inline void _to_vector(std::vector<T>& vec, const T& item, const TD&... items) {
   vec.push_back(item);
   _to_vector(vec, items...);
}

//-------------------------------------------------------------------
template<typename T, typename... TD>
inline std::vector<T> to_vector(const T& item, const TD&... items) {
   std::vector<T> vec;
   _to_vector(vec, item, items...);
   return vec;
}
}

}


#endif /* !__MOONLIGHT_VARIADIC_H */
