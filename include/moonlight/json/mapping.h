/*
 * mapping.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Monday September 13, 2021
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_JSON_MAPPING_H
#define __MOONLIGHT_JSON_MAPPING_H

#include <functional>
#include <type_traits>
#include <string>
#include <utility>
#include <vector>
#include "moonlight/json/object.h"

namespace moonlight {
namespace json {

// ------------------------------------------------------------------
class Mapping {
 public:
     typedef std::unique_ptr<Mapping> Pointer;

     Mapping(const char* name, bool required)
     : _name(name), _required(required) { }

     virtual ~Mapping() { }

     virtual Value::Pointer get() const = 0;
     virtual void set(Value::Pointer value) const = 0;

     std::string name() const {
         return _name;
     }

     bool required() const {
         return _required;
     }

 private:
     const char* _name;
     bool _required;
};

//-------------------------------------------------------------------
template<typename T>
class PropertyMapping : public Mapping {
 public:
     typedef typename std::remove_const<T>::type Type;
     typedef std::function<const Type()> Getter;
     typedef std::function<void(const Type&)> Setter;

     PropertyMapping(const char* name,
                     Getter getter,
                     Setter setter,
                     bool required)
     : Mapping(name, required), _getter(getter), _setter(setter) { }

     Value::Pointer get() const override {
         return Value::of(_getter());
     }

     void set(Value::Pointer value) const override {
         if (! value->is<Type>()) {
             THROW(core::TypeError,
                   "Can't save value of type " + value->type_name() + "to the \"" + name() + "\" property mapping.");
         }

         _setter(value->get<Type>());
     }

 private:
     Getter _getter;
     Setter _setter;
};

//-------------------------------------------------------------------
template<typename T>
class FieldMapping : public Mapping {
 public:
     typedef typename std::remove_const<T>::type Type;

     FieldMapping(const char* name, T& field, bool required)
     : Mapping(name, required), _field(field) { }

     Value::Pointer get() const override {
         return Value::of(_field);
     }

     void set(Value::Pointer value) const override {
         if (! value->is<Type>()) {
             THROW(core::TypeError,
                   "Can't save value of type " + value->type_name() + "to the \"" + name() + "\" field mapping.");
         }

         _field = value->get<Type>();
     }

 private:
     T& _field;
};

//-------------------------------------------------------------------
template<typename T>
class ObjectMapping : public Mapping {
 public:
     typedef typename std::remove_const<T>::type Type;

     ObjectMapping(const char* name, Type& obj_ref, bool required)
     : Mapping(name, required), _obj_ref(obj_ref) { }

     Value::Pointer get() const override {
         auto obj = std::make_shared<Object>();

         for (auto mapping : _obj_ref.json_mapper()) {
             obj->set(mapping->name(), Value::of(mapping->get()));
         }

         return obj;
     }

     void set(Value::Pointer value) const override {
         if (! value->is<Object>()) {
             THROW(core::TypeError, "Can't apply non-object mapping to \"" + name() + "\" object.");
         }
     }

 private:
     Type& _obj_ref;
};

//-------------------------------------------------------------------
template<class C>
class Mapper {
 public:
     explicit Mapper(C* instance) : _instance(instance) { }
     Mapper(Mapper& other)
     : _mappings(std::move(other._mappings)), _instance(other._instance) { }


     template<class UnboundGetter, class UnboundSetter>
     Mapper& property(const char* name,
                      const UnboundGetter& getter,
                      const UnboundSetter& setter,
                      bool required = false) {
         using namespace std::placeholders;

         typedef std::remove_reference_t<decltype(std::invoke(getter, _instance))> Type;

         C* instance = _instance;
         auto getter_proxy = [=]() -> Type {
             const auto bound_getter = [=]() { return std::invoke(getter, instance); };
             return bound_getter();
         };

         auto setter_proxy = [=](const Type& p) {
             const auto bound_setter = [=](const Type& t) { std::invoke(setter, instance, t); };
             bound_setter(p);
         };

         _mappings.push_back(std::make_unique<PropertyMapping<Type>>(
                 name,
                 getter_proxy,
                 setter_proxy,
                 required));
         return *this;
     }

     template<class T>
     Mapper& field(const char* name, T& ref, bool required = false) {
         _mappings.push_back(std::make_unique<FieldMapping<T>>(
                 name, ref, required));
         return *this;
     }

     json::Object map_to_json() const {
         json::Object obj;
         for (auto& mapping : _mappings) {
             obj.set(mapping->name(), mapping->get());
         }
         return obj;
     }

     C& map_from_json(const json::Object& obj) const {
         for (auto& mapping : _mappings) {
             if (mapping->required() && !obj.contains(mapping->name())) {
                 THROW(core::TypeError, "Missing required field \"" + mapping->name() + "\" on JSON object.");
             }

             auto value = obj.get<Value::Pointer>(mapping->name());
             if (value != nullptr) {
                 mapping->set(obj.get<Value::Pointer>(mapping->name()));
             }
         }
         return *_instance;
     }

 private:
     std::vector<Mapping::Pointer> _mappings;
     C* _instance;
};

}  // namespace json
}  // namespace moonlight

#endif /* !__MOONLIGHT_JSON_MAPPING_H */
