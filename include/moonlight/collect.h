/*
 * ## collect.h: Functional tools for collections. ------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library provides a variety of functional template methods that work on
 * many different STL collection types.  These serve as a supplement and
 * occasional alternative to the STL `<algorithm>` methods.
 *
 * The following functions are defined in the `moonlight::coll` namespace:
 *
 * - `contains(C, x)`: Determine if the collection `C` contains the value `x`.
 * - `flatten(C)`: Flattens an arbitrarily nested collection into a single
 *   non-nested collection.
 * - `filter(C, f(x))`: Filter the collection into a new collection only
 *   containing elements for which the `f(x)` predicate is true.
 * - `sorted(C)`: Return a new collection containing the elements of `C` in
 *   sorted order.
 * - `map(C, f(x))`: Return a new collection containing the elements of `C`
 *   transformed by the function `f(x)`.
 * - `set(C)`: Copy the elements from `C` into a new `std::set`, effectively
 *   filtering duplicates from `C` into a new set.
 * - `zip<R>(A, B)`: Combine the contents of `A` and `B` into a single new
 *   `std::vector<R>` by constructing new `R` objects using elements `A[x]` and
 *   `B[x]`, i.e. `R(A[x], B[x])`.
 */

#ifndef __MOONLIGHT_COLLECT_H
#define __MOONLIGHT_COLLECT_H

#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <memory>

namespace moonlight {
namespace collect {

template<typename T>
inline bool contains(const T& coll, const typename T::value_type& v) {
    return std::find(coll.begin(), coll.end(), v) != coll.end();
}

template<typename T>
inline bool contains(const std::set<T>& set, const T& v) {
    return set.find(v) != set.end();
}

template<typename K, typename V>
inline bool contains(const std::map<K, V>& map, const K& v) {
    return map.contains(v);
}

template<typename T, typename... TV>
inline std::vector<typename T::value_type> flatten(const T& coll, const TV&... collections) {
    std::vector<typename T::value_type> flattened;
    flatten(flattened, coll, collections...);
    return flattened;
}

template<typename T, typename... TV>
inline void flatten(std::vector<typename T::value_type>& flattened, const T& coll, const TV&... collections) {
    for (auto v : coll) {
        flattened.push_back(v);
    }
    flatten(flattened, collections...);
}

template<typename T>
inline void flatten(std::vector<T>& flattened) { (void) flattened; }

template<typename T>
inline T filter(const T& coll, const std::function<bool(typename T::value_type)>& f) {
    T result;
    for (auto v : coll) {
        if (f(v)) {
            result.push_back(v);
        }
    }
    return result;
}

template<typename T>
inline std::shared_ptr<T> filter(const std::shared_ptr<T>& coll,
                                 const std::function<bool(typename T::value_type)>& f) {
    return std::make_shared<T>(filter<typename std::shared_ptr<T>::element_type>(*coll, f));
}

template<typename C1>
inline C1 sorted(const C1& src) {
    C1 result;
    std::copy(src.begin(), src.end(), std::back_inserter(result));
    std::sort(result.begin(), result.end());
    return result;
}

template<typename C1>
inline C1 sorted(const C1& src, std::function<bool(const typename C1::value_type& a,
                                                   const typename C1::value_type& b)> comp) {
    C1 result;
    std::copy(src.begin(), src.end(), std::back_inserter(result));
    std::sort(result.begin(), result.end(), comp);
    return result;
}

template<typename T, typename C1>
inline std::vector<T> map(const C1& src, std::function<T(const typename C1::value_type&)> f) {
    std::vector<T> result;
    for (auto v : src) {
        result.push_back(f(v));
    }
    return result;
}

template<typename C>
inline std::set<typename C::value_type> set(const C& src) {
    std::set<typename C::value_type> result;
    std::copy(src.begin(), src.end(), std::back_inserter(result));
    return result;
}

template<class R, class C1, class C2>
inline std::vector<R> zip(const C1& srcA, const C2& srcB) {
    std::vector<R> result;
    if (srcA.size() >= srcB.size()) {
        std::transform(srcA.begin(), srcA.end(), srcB.begin(), std::back_inserter(result),
                       [](const auto& a, const auto& b) {
                           return R(a, b);
                       });
    } else {
        std::transform(srcB.begin(), srcB.end(), srcA.begin(), std::back_inserter(result),
                       [](const auto& b, const auto& a) {
                           return R(a, b);
                       });
    }

    return result;
}


}  // namespace collect
}  // namespace moonlight

#endif /* !__MOONLIGHT_COLLECT_H */
