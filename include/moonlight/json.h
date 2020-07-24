/*
 * moonlight/json.h: JSON wrapper class for simplified object serialization.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Wednesday, Jan 14 2015,
 *       Thursday, Dec 21 2017
 */
#ifndef __MOONLIGHT_JSON_H
#define __MOONLIGHT_JSON_H

#include "moonlight/exceptions.h"
#include "moonlight/file.h"
#include "moonlight/maps.h"
#include "picojson/picojson.h"

#include <memory>
#include <sstream>

namespace moonlight {
namespace json {

//-------------------------------------------------------------------
class WrapperException : public core::Exception {
public:
   using core::Exception::Exception;
};

namespace json_impl {
//-------------------------------------------------------------------
template <class T>
void set_value(picojson::value& obj_value,
               const std::string& name,
               const T& value) {
   picojson::object& obj = obj_value.get<picojson::object>();
   picojson::value json_value(value);
   obj[name] = json_value;
}

//-------------------------------------------------------------------
template <class T>
T get_value(const picojson::value& obj_value,
            const std::string& name) {
   if (! obj_value.contains(name)) {
      throw WrapperException(str::cat(
         "Missing value for key '", name, "'."
      ));
   }

   picojson::value value = obj_value.get(name);
   if (! value.is<T>()) {
      throw WrapperException(str::cat(
         "Unexpected value type for key '", name, "'."
      ));
   }

   return value.get<T>();
}

//-------------------------------------------------------------------
template <class T>
T get_value_or_default(const picojson::value& obj_value,
                       const std::string& name,
                       const T& default_value) {
   try {
      return get_value<T>(obj_value, name);

   } catch (const WrapperException& e) {
      return default_value;
   }
}

//-------------------------------------------------------------------
template <class T>
T get_value(picojson::value& obj_value,
            const std::string& name,
            const T& default_value) {
   try {
      return get_value<T>(obj_value, name);

   } catch (const WrapperException& e) {
      set_value(obj_value, name, default_value);
      return default_value;
   }
}

//-------------------------------------------------------------------
template <class T>
void set_array(picojson::value& obj_value,
               const std::string& name,
               const std::vector<T>& vec) {
   picojson::object& obj = obj_value.get<picojson::object>();
   picojson::array array;

   for (T val : vec) {
      array.push_back(picojson::value(val));
   }

   obj[name] = picojson::value(array);
}

//-------------------------------------------------------------------
template<class T>
std::vector<T> get_array(const picojson::value& obj_value,
                         const std::string& name) {
   std::vector<T> vec;

   if (! obj_value.contains(name)) {
      throw WrapperException(str::cat(
         "Missing array for key '", name, "'."
      ));
   }

   const picojson::value& array_value = obj_value.get(name);
   if (! array_value.is<picojson::array>()) {
      throw WrapperException(str::cat(
         "Key '", name, "' does not refer to an array."
      ));
   }

   const picojson::array& array = array_value.get<picojson::array>();
   for (picojson::value val : array) {
      if (! val.is<T>()) {
         throw WrapperException(str::cat(
            "Unexpected heterogenous value type in array for key '", name, "'."
         ));
      }

      vec.push_back(val.get<T>());
   }

   return vec;
}

//-------------------------------------------------------------------
template <class T>
std::vector<T> get_array(picojson::value& obj_value,
                         const std::string& name,
                         const std::vector<T>& default_array) {
   try {
      return get_array<T>(obj_value, name);

   } catch (const WrapperException& e) {
      set_array(name, default_array);
      return default_array;
   }
}

//-------------------------------------------------------------------
template <>
inline int get_value<int>(const picojson::value& obj_value,
                          const std::string& name) {
   return (int)get_value<double>(obj_value, name);
}

//-------------------------------------------------------------------
template <>
inline int get_value<int>(picojson::value& obj_value,
                          const std::string& name,
                          const int& default_value) {
   return (int)(get_value<double>(obj_value, name,
                                  (double)(default_value)));
}

//-------------------------------------------------------------------
template <>
inline void set_array<int>(picojson::value& obj_value,
                           const std::string& name,
                           const std::vector<int>& vec) {
   std::vector<double> db_vec;
   copy(vec.begin(), vec.end(), back_inserter(db_vec));
   set_array<double>(obj_value, name, db_vec);
}

//-------------------------------------------------------------------
template <>
inline std::vector<int> get_array<int>(const picojson::value& obj_value,
                                       const std::string& name) {
   std::vector<int> vec;
   std::vector<double> db_vec = get_array<double>(obj_value, name);
   copy(db_vec.begin(), db_vec.end(), back_inserter(vec));
   return vec;
}

//-------------------------------------------------------------------
template <>
inline std::vector<int> get_array<int>(picojson::value& obj_value,
                                       const std::string& name,
                                       const std::vector<int>& default_vec) {
   try {
      return get_array<int>(obj_value, name);

   } catch (const WrapperException& e) {
      set_array(obj_value, name, default_vec);
      return default_vec;
   }
}
}

/**------------------------------------------------------------------
 * A wrapper class around picojson with a simplified, object-focused interface.
 *
 * NOTE: This class does not offer direct access to the following structures:
 *    - Documents which are not objects, i.e. list documents.
 *    - Heterogenous lists, i.e. lists must contain only objects, ints, floats,
 *      bools, or strings, and may not mix these data types.
 *    - Lists of lists
 *
 * These limitations are by design and allow for a greatly simplified interface
 * that's useful for settings and object serialization.
 *
 * If such objects need to be accessed, please use picojson directly.
 * You can also load objects that contain forbidden structures, then use
 * Wrapper::get to fetch the wrapped picojson::value and inspect further there.
 */
class Wrapper {
public:
   Wrapper() : obj_value(std::make_shared<picojson::value>(picojson::object())) { }
   Wrapper(const std::shared_ptr<picojson::value>& obj_value) :
   obj_value(obj_value) { }

   virtual ~Wrapper() { }

   static Wrapper load_from_file(const std::string& filename) {
      try {
         std::ifstream infile = file::open_r(filename);
         return load_from_stream(infile);
      } catch (const WrapperException& e) {
         throw WrapperException(str::cat(
            "File does not contain a JSON object: '", filename, "'."
         ));
      }
   }

   static Wrapper load_from_string(const std::string& json_string) {
      std::istringstream sb(json_string);

      try {
         return load_from_stream(sb);
      } catch (const WrapperException& e) {
         throw WrapperException("JSON string does not contain an object.");
      }
   }

   static Wrapper load_from_stream(std::istream& infile) {
      std::shared_ptr<picojson::value> obj_value = std::make_shared<picojson::value>();
      infile >> (*obj_value);
      if (! obj_value->is<picojson::object>()) {
         throw WrapperException("Stream did not contain a JSON object.");
      }
      return Wrapper(obj_value);
   }

   void save_to_file(const std::string& filename, bool prettify = false) const {
      std::ofstream outfile = file::open_w(filename);
      print(outfile, prettify);
   }

   void print(std::ostream& outfile, bool prettify = false) const {
      outfile << to_string(prettify);
   }

   std::string to_string(bool prettify = false) const {
      return obj_value->serialize(prettify);
   }

   bool contains(const std::string& name) const {
      return obj_value->contains(name);
   }

   std::vector<std::string> get_keys() const {
      return maps::keys(obj_value->get<picojson::object>());
   }

   picojson::value& get() {
      return *obj_value;
   }

   const picojson::value& get() const {
      return *obj_value;
   }

   template <class T>
   T get(const std::string& name) const {
      return json_impl::get_value<T>(
          *std::const_pointer_cast<const picojson::value>(obj_value), name);
   }

   template <class T>
   T get(const std::string& name, const T& default_value) {
      return json_impl::get_value<T>(get(), name, default_value);
   }

   template <class T>
   T get_default(const std::string& name, const T& default_value) const {
      return json_impl::get_value_or_default(
          *std::const_pointer_cast<const picojson::value>(obj_value),
          name, default_value);
   }

   template <class T>
   void set(const std::string& name, const T& value) {
      json_impl::set_value<T>(get(), name, value);
   }

   template <class T>
   std::vector<T> get_array(const std::string& name) const {
      return json_impl::get_array<T>(get(), name);
   }

   template <class T>
   std::vector<T> get_array(const std::string& name,
                            const std::vector<T>& default_vec) {
      return json_impl::get_array<T>(get(), name, default_vec);
   }

   template <class T>
   void set_array(const std::string& name, const std::vector<T>& vec) {
      json_impl::set_array<T>(get(), name, vec);
   }

   std::vector<Wrapper> get_object_array(const std::string& name) const {
      picojson::value obj_values_array_value = obj_value->get(name);

      if (! obj_values_array_value.is<picojson::array>()) {
         throw WrapperException(str::cat(
            "Key '", name, "' does not refer to an object array."
         ));
      }

      picojson::array& obj_values_array = obj_values_array_value.get<picojson::array>();
      std::vector<Wrapper> obj_array;

      for (picojson::value val : obj_values_array) {
         if (! val.is<picojson::object>()) {
            throw WrapperException(str::cat(
               "JSON array contains non-object: '", name, "'."
            ));
         }

         obj_array.push_back(Wrapper(std::make_shared<picojson::value>(val)));
      }

      return obj_array;
   }

   void set_object_array(const std::string& name, const std::vector<Wrapper>& obj_list) {
      picojson::array array;
      picojson::object& obj = obj_value->get<picojson::object>();

      for (Wrapper obj : obj_list) {
         array.push_back(*(obj.obj_value.get()));
      }

      obj[name] = picojson::value(array);
   }

   Wrapper get_object(const std::string& name, bool must_exist = false) const {
      if (! obj_value->contains(name)) {
         if (must_exist) {
            throw WrapperException(str::cat(
               "Missing object for key '", name, "'."
            ));
         } else {
            return Wrapper();
         }
      }

      picojson::value object_json = obj_value->get(name);
      if (! object_json.is<picojson::object>()) {
         throw WrapperException(str::cat(
            "Key '", name, "' does not refer to an object."
         ));
      }

      return Wrapper(std::make_shared<picojson::value>(object_json));
   }

   void set_object(const std::string& name, const Wrapper& object) {
      picojson::object& obj = obj_value->get<picojson::object>();
      obj[name] = (*object.obj_value);
   }

   friend std::ostream& operator<<(std::ostream& out, const Wrapper& settings) {
      out << settings.to_string();
      return out;
   }

private:
   std::shared_ptr<picojson::value> obj_value;
};

namespace json_impl {
//-------------------------------------------------------------------
template <>
inline std::vector<Wrapper> get_array<Wrapper>(const picojson::value& obj_value, const std::string& name) {
   std::vector<picojson::object> obj_array = get_array<picojson::object>(obj_value, name);
   std::vector<Wrapper> json_vec(obj_array.size());

   for (auto obj : obj_array) {
      json_vec.push_back(Wrapper(std::make_shared<picojson::value>(obj)));
   }

   return json_vec;
}
}

//-------------------------------------------------------------------
inline Wrapper load_from_file(const std::string& filename) {
   return Wrapper::load_from_file(filename);
}

//-------------------------------------------------------------------
inline Wrapper load_from_string(const std::string& json_string) {
   return Wrapper::load_from_string(json_string);
}

//-------------------------------------------------------------------
inline Wrapper load_from_stream(std::istream& infile) {
   return Wrapper::load_from_stream(infile);
}

}
}

#endif /*__MOONLIGHT_JSON_H */
