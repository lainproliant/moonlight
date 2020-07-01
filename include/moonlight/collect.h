/*
 * collect.h: Functional tools for collections.
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_COLLECT_H
#define __MOONLIGHT_COLLECT_H

#include <vector>
#include <set>
#include <map>
#include <functional>
#include <memory>

namespace moonlight {
namespace collect {

//-------------------------------------------------------------------
template<typename T>
inline bool contains(const T& coll, const typename T::value_type& v) {
   return std::find(coll.begin(), coll.end(), v) != coll.end();
}

//-------------------------------------------------------------------
template<typename T>
inline bool contains(const std::set<T>& set, const T& v) {
   return set.find(v) != set.end();
}

//-------------------------------------------------------------------
template<typename K, typename V>
inline bool contains(const std::map<K, V>& map, const K& v) {
   return map.find(v) != map.end();
}

//-------------------------------------------------------------------
template<typename T, typename... TV>
inline std::vector<typename T::value_type> flatten(const T& coll, const TV&... collections) {
   std::vector<typename T::value_type> flattened;
   flatten(flattened, coll, collections...);
   return flattened;
}

//-------------------------------------------------------------------
template<typename T, typename... TV>
inline void flatten(std::vector<typename T::value_type>& flattened, const T& coll, const TV&... collections) {
   for (auto v : coll) {
      flattened.push_back(v);
   }
   flatten(flattened, collections...);
}

//-------------------------------------------------------------------
template<typename T>
inline void flatten(std::vector<T>& flattened) { (void) flattened; }

//-------------------------------------------------------------------
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

//-------------------------------------------------------------------
template<typename T>
inline std::shared_ptr<T> filter(const std::shared_ptr<T>& coll,
                                 const std::function<bool(typename T::value_type)>& f) {
   return std::make_shared<T>(filter<typename std::shared_ptr<T>::element_type>(*coll, f));
}

//-------------------------------------------------------------------
template<typename C1>
inline C1 sorted(const C1& src) {
   C1 result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   std::sort(result.begin(), result.end());
   return result;
}

//-------------------------------------------------------------------
template<typename C1>
inline C1 sorted(const C1& src, std::function<bool (const typename C1::value_type& a,
                                                    const typename C1::value_type& b)> comp) {
   C1 result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   std::sort(result.begin(), result.end(), comp);
   return result;
}

//-------------------------------------------------------------------
template<typename T, typename C1>
inline std::vector<T> map(const C1& src, std::function<T (const typename C1::value_type&)> f) {
   std::vector<T> result;
   for (auto v : src) {
      result.push_back(f(v));
   }
   return result;
}

//-------------------------------------------------------------------
template<typename C>
inline std::set<typename C::value_type> set(const C& src) {
   std::set<typename C::value_type> result;
   std::copy(src.begin(), src.end(), std::back_inserter(result));
   return result;
}


}
}


#endif /* !__MOONLIGHT_COLLECT_H */
