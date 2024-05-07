/*
 * functional_map.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Wednesday May 1, 2024
 */

#ifndef __FUNCTIONAL_MAP_H
#define __FUNCTIONAL_MAP_H

#include <functional>
#include "moonlight/exceptions.h"

namespace moonlight {

EXCEPTION_SUBTYPE(core::ValueError, UnmappedValueError);

template<class T, class M = T>
class FunctionalMap {
 public:
     typedef T key_type;
     typedef M value_type;
     typedef std::function<M(const T&)> function_f;
     typedef std::function<bool(const T&)> predicate_f;

     struct Mapping {
         std::vector<predicate_f> predicates = {};
         function_f func = [](auto& v) -> int {
             THROW(UnmappedValueError, "Undefined mapping.");
         };

         Mapping() { }

         Mapping(const std::vector<predicate_f>& predicates) {
             this->predicates = predicates;
         }

         Mapping& operator=(function_f func) {
             this->func = func;
             return *this;
         }

         Mapping& operator=(const M& value) {
             *this = [=](auto& v) { return value; };
             return *this;
         }

         bool check(const T& value) const {
             for (auto predicate : predicates) {
                 if (predicate(value)) {
                     return true;
                 }
             }
             return false;
         }
     };

     function_f operator[](const T& value) const {
         return _of(value).value_or(_otherwise.func);
     }

     template<class... VL>
     Mapping& operator[](VL... args) {
         auto predicates = _build_predicates(args...);
         _mappings.emplace_back(Mapping(predicates));
         return _mappings.back();
     }

     Mapping& otherwise() {
         return _otherwise;
     }

     bool contains(const T& value) const {
         auto func = _of(value);
         return func.has_value();
     }

     function_f of(const T& value) const {
         return (*this)[value];
     }

     M operator()(const T& value) const {
         return of(value)(value);
     }

 private:
     std::optional<function_f> _of(const T& value) const {
         for (auto& mapping : _mappings) {
             if (mapping.check(value)) {
                 return mapping.func;
             }
         }

         return {};
     }

     template<class V, class... VL>
     std::vector<predicate_f> _build_predicates(const V& value, VL... args) {
         std::vector<predicate_f> predicates;
         _build(predicates, value, args...);
         return predicates;
     }

     template<class... VL>
     void _build(std::vector<predicate_f>& predicates, const T& value, VL... args) {
         predicates.push_back([=](const T& other) { return value == other; });
         _build(predicates, args...);
     }

     template<class... VL>
     void _build(std::vector<predicate_f>& predicates, predicate_f predicate, VL... args) {
         predicates.push_back(predicate);
         _build(predicates, args...);
     }

     void _build(std::vector<predicate_f>& predicates) { }

     std::vector<Mapping> _mappings;
     Mapping _otherwise;
};

}

#endif /* !__FUNCTIONAL_MAP_H */
