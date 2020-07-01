/*
 * mmap.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_MMAP_H
#define __MOONLIGHT_MMAP_H

#include <vector>
#include <map>

namespace moonlight {
namespace mmap {

/**------------------------------------------------------------------
 * Template class used as parameter type for mmaps::build().
 */
template<typename K, typename T>
struct mapping {
   K key;
   std::vector<T> values;
};

/**------------------------------------------------------------------
 * Build a multimap from the given constant mapping.  Intended for use
 * in statically defining multimaps, e.g.:
 *
 * ```
 * static const auto mmap = build({
 *    {"even",    {2, 4, 6, 8}},
 *    {"odd",     {1, 3, 5, 7}}
 * });
 * ```
 */
template<typename K, typename T>
inline std::multimap<K, T> build(const std::vector<mapping<K, T>>& mappings) {
   std::multimap<K, T> mmap;
   for (auto mapping : mappings) {
      for (auto value : mapping.values) {
         mmap.insert(std::make_pair(mapping.key, value));
      }
   }

   return mmap;
}

/**------------------------------------------------------------------
 * Variadic version of mmap::build().
 *
 * ```
 * static const auto mmap = build(
 *    "even",    {2, 4, 6, 8},
 *    "odd",     {1, 3, 5, 7}
 * );
 * ```
 */
template<typename K, typename T, typename... KTV>
inline std::multimap<K, T> build(const K& key, const T& value, const KTV&... mappings) {
   std::multimap<K, T> mmap;
   build(mmap, key, value, mappings...);
   return mmap;
}

//-------------------------------------------------------------------
template<typename K, typename T, typename... KTV>
inline void build(std::multimap<K, T>& mmap, const K& key, const T& value,
                  const KTV&... mappings) {
   mmap.insert({key, value});
   build(mmap, mappings...);
}

template<typename K, typename T>
inline void build(std::multimap<K, T>& mmap) { (void) mmap; }

/**------------------------------------------------------------------
 * Collect all values from the given multimap-like iterable that match
 * the given key.
 */
template<typename M>
inline std::vector<typename M::mapped_type> collect(const M& mmap,
                                                    const typename M::key_type& key) {
   std::vector<typename M::mapped_type> values;
   auto range = mmap.equal_range(key);

   for (auto iter = range.first; iter != range.second; iter++) {
      values.insert(values.end(), iter->second);
   }

   return values;
}

}
}


#endif /* !__MOONLIGHT_MMAP_H */
