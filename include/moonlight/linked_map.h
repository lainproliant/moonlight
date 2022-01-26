/*
 * linked_map
 *
 * An efficient map aggregate container that preserves the insertion
 * order of associated key/value pairs.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: June 4th, 2013, October 3rd, 2021
 *
 * Distributed under terms of the MIT license.
 */
#ifndef __MOONLIGHT_LINKED_MAP_H
#define __MOONLIGHT_LINKED_MAP_H

#include <map>
#include <unordered_map>
#include <algorithm>
#include <iterator>
#include <list>

namespace moonlight {

//-------------------------------------------------------------------
template<class K, class V,
         class L = std::list<std::pair<K, V>>,
         class M = std::unordered_map<K, typename L::iterator>>
class LinkedMap {
public:
    typedef typename M::key_type key_type;
    typedef V mapped_type;
    typedef typename L::value_type value_type;
    typedef typename M::size_type size_type;
    typedef typename M::difference_type difference_type;
    typedef typename M::hasher hasher;
    typedef typename M::key_equal key_equal;
    typedef typename M::allocator_type allocator_type;
    typedef typename L::reference reference;
    typedef typename L::const_reference const_reference;
    typedef typename L::pointer pointer;
    typedef typename L::const_pointer const_pointer;
    typedef typename L::iterator iterator;
    typedef typename L::const_iterator const_iterator;
    typedef typename L::reverse_iterator reverse_iterator;
    typedef typename L::const_reverse_iterator const_reverse_iterator;

    LinkedMap() { }
    LinkedMap(const LinkedMap& other) : _map(other._map), _list(other._list) { }
    ~LinkedMap() { }

    LinkedMap& operator=(const LinkedMap& other) {
        _list.clear();
        _map.clear();
        for (auto iter : other) {
            insert(iter);
        }
        return *this;
    }

    allocator_type get_allocator() const noexcept {
        return _map.get_allocator();
    }

    bool empty() const {
        return _list.empty();
    }

    size_t size() const {
        return _list.size();
    }

    size_t max_size() const {
        return std::min(_map.max_size(), _list.max_size());
    }

    void clear() {
        _map.clear();
        _list.clear();
    }

    std::pair<iterator, bool> insert(const value_type& value) {
        auto result = _map.find(value.first);
        if (result != _map.end()) {
            return {result->second, false};
        }

        _list.push_back(value);
        auto iter = std::prev(_list.end());
        _map.emplace(value.first, iter);
        return {iter, true};
    }

    iterator insert(const_iterator hint, const value_type& value) {
        (void) hint;
        auto result = _map.find(value.first);
        if (result != _map.end()) {
            return result->second;
        }

        _list.push_back(value);
        auto iter = std::prev(_list.end());
        _map.emplace(value.first, iter);
        return iter;
    }

    std::pair<iterator, bool> insert_or_assign(const value_type& value) {
        auto result = _map.find(value.first);
        if (result != _map.end()) {
            result->first->second = value.second;
            return {result->second, false};
        }

        return insert(value);
    }

    template<class... TD>
    std::pair<iterator, bool> emplace(const TD&&... args) {
        auto iter = _list.emplace_back(std::forward<value_type>(args)...);
        auto result = _map.find(iter->first);
        if (result != _map.end()) {
            _list.pop_back();
            return {result->second, false};
        }

        _map.emplace(iter->first, iter);
        return {iter, true};
    }

    template<class... TD>
    std::pair<iterator, bool> try_emplace(const key_type& key, const TD&&... args) {
        auto result = _map.find(key);
        if (result != _map.end()) {
            return emplace(std::forward(args)...);
        }

        return {result->second, false};
    }

    iterator erase(const_iterator iter) {
        _map.erase(iter->first);
        return _list.erase(iter);
    }

    iterator erase(iterator iter) {
        _map.erase(iter->first);
        return _list.erase(iter);
    }

    iterator erase(const_iterator first, const_iterator last) {
        for (auto iter = first; iter != last; iter = erase(iter));
    }

    size_type erase(const key_type& key) {
        auto result = _map.find(key);
        if (result != _map.end()) {
            erase(result->second);
            return 1;
        }

        return 0;
    }

    void swap(LinkedMap<K, V, L, M>& other) {
        std::swap(_map, other._map);
        std::swap(_list, other._list);
    }

    mapped_type& at(const key_type& key) {
        auto result = _map.find(key);
        if (result == _map.end()) {
            throw std::out_of_range("Key not found in map.");
        }

        return result->second->second;
    }

    const mapped_type& at(const key_type& key) const {
        auto result = _map.find(key);
        if (result == _map.end()) {
            throw std::out_of_range("Key not found in map.");
        }

        return result->second->second;
    }

    mapped_type& at_offset(size_t offset) {
        if (_list.size() >= offset) {
            throw std::out_of_range("Offset is beyond the end of the container.");
        }
        return _list.at(offset).second;
    }

    const mapped_type& at_offset(size_t offset) const {
        if (_list.size() >= offset) {
            throw std::out_of_range("Offset is beyond the end of the container.");
        }
        return _list.at(offset).second;
    }

    template<class T>
    mapped_type& operator[](const T& key_or_offset) {
        return at(key_or_offset);
    }

    template<class KeyEquiv>
    size_t count(const KeyEquiv& key) const {
        return _map.count(key);
    }

    template<class KeyEquiv>
    bool contains(const KeyEquiv& key) const {
        return _map.contains(key);
    }

    template<class KeyEquiv>
    iterator find(const KeyEquiv& key) {
        auto map_iter = _map.find(key);
        if (map_iter == _map.end()) {
            return end();
        }
        return map_iter->second;
    }

    template<class KeyEquiv>
    const_iterator find(const KeyEquiv& key) const {
        auto map_iter = _map.find(key);
        if (map_iter == _map.end()) {
            return end();
        }
        return map_iter->second;
    }

    template<class KeyEquiv>
    std::pair<iterator, iterator> equal_range(const KeyEquiv& key) {
        iterator first_equal = _list.end();

        for (auto iter = _list.begin(); iter != _list.end(); iter++) {
            if (key_eq()(iter->first, key) && first_equal == _list.end()) {
                first_equal = iter;

            } else if (! key_eq()(iter->first, key) && first_equal != _list.end()) {
                return {first_equal, iter};
            }
        }

        return {first_equal, _list.end()};
    }

    template<class KeyEquiv>
    std::pair<const_iterator, const_iterator> equal_range(const KeyEquiv& key) const {
        const_iterator first_equal = _list.end();

        for (auto iter = _list.begin(); iter != _list.end(); iter++) {
            if (key_eq()(iter->first, key) && first_equal == _list.end()) {
                first_equal = iter;

            } else if (! key_eq()(iter->first, key) && first_equal != _list.end()) {
                return {first_equal, iter};
            }
        }

        return {first_equal, _list.end()};
    }

    const L& list() const {
        return _list;
    }

    iterator begin() {
        return _list.begin();
    }

    const_iterator begin() const {
        return _list.begin();
    }

    const_iterator cbegin() const {
        return begin();
    }

    iterator end() {
        return _list.end();
    }

    const_iterator end() const {
        return _list.end();
    }

    const_iterator cend() const {
        return cend();
    }

    size_type bucket_count() const {
        return _map.bucket_count();
    }

    size_type max_bucket_count() const {
        return _map.max_bucket_count();
    }

    size_type bucket_size(size_type n) const {
        return _map.bucket_size(n);
    }

    size_type bucket(const key_type& key) const {
        return _map.bucket(key);
    }

    float load_factor() const {
        return _map.load_factor();
    }

    float max_load_factor() const {
        return _map.max_load_factor();
    }

    void max_load_factor(float factor) const {
        _map.max_load_factor(factor);
    }

    void rehash(size_type count) const {
        _map.rehash(count);
    }

    void reserve(size_type count) const {
        _map.reserve(count);
    }

    hasher hash_function() const {
        return _map.hash_function();
    }

    key_equal key_eq() const {
        return _map.key_eq();
    }

    bool operator==(const LinkedMap<K, V, L, M>& other) const {
        return _list == other._list;
    }

private:
    M _map;
    L _list;
};

//-------------------------------------------------------------------
// Template alias for backwards compatibility.
template<class K, class V,
         class L = std::list<std::pair<K, V>>,
         class M = std::unordered_map<K, typename L::iterator>>
using linked_map = LinkedMap<K, V, L, M>;

}

#endif /* __MOONLIGHT_LINKED_MAP_H */
