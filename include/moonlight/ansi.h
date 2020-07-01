#ifndef __MOONLIGHT_ANSI_H
#define __MOONLIGHT_ANSI_H

#include "moonlight/string.h"
#include "moonlight/exceptions.h"
#include <unordered_map>

namespace moonlight {
namespace ansi {
/**------------------------------------------------------------------
 * Join the given iterable collection into a token delimited string.
 */
template<typename T>
std::string join(const T& coll, const std::string& token = "") {
   std::ostringstream sb;

   for (typename T::const_iterator i = coll.begin(); i != coll.end();
        i++) {
      if (i != coll.begin()) sb << std::string(token);
      sb << *i;
   }

   return sb.str();
}

//-------------------------------------------------------------------
std::string seq(const std::string& s) {
   return "\033[" + s;
}

//-------------------------------------------------------------------
void _attr(std::vector<std::string>& vec) { (void) vec; }

//-------------------------------------------------------------------
template<class T, class... Elements>
void _attr(std::vector<std::string>& vec, const T& element,
           Elements... elements) {
   vec.push_back(std::to_string(element));
   _attr(vec, elements...);
}

//-------------------------------------------------------------------
template<class T, class... Elements>
std::string attr(const T& element, Elements... elements) {
   std::vector<std::string> vec;
   _attr(vec, element, elements...);
   return seq(join(vec, ";") + "m");
}

//-------------------------------------------------------------------
const std::string empty = "";
const std::string clrscr = seq("2J") + seq("0;0H");
const std::string clreol = seq("K");
const std::string reset = attr(0);
const std::string bright = attr(1);
const std::string dim = attr(2);
const std::string underscore = attr(4);
const std::string blink = attr(5);
const std::string reverse = attr(7);
const std::string hidden = attr(8);
const std::string fraktur = attr(20);

//-------------------------------------------------------------------
class Decorator {
public:
   Decorator(const std::string& before = empty, const std::string& after = empty)
   : before(before), after(after) { }
   virtual ~Decorator() { }

   Decorator compose(const Decorator& other) {
      Decorator new_decorator(before, after);
      if (other.before != before) new_decorator.before = before + other.before;
      if (other.after != after) new_decorator.after = other.after + after;
      return new_decorator;
   }

   std::string operator()(const std::string& str = empty) {
      return before + str + after;
   }

private:
   std::string before;
   std::string after;
};

//-------------------------------------------------------------------
const auto none = Decorator();

//-------------------------------------------------------------------
typedef std::unordered_map<std::string, Decorator> DecoratorMap;

//-------------------------------------------------------------------
const std::vector<std::string> COLORS = {
   "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white" };

//-------------------------------------------------------------------
class Library {
public:
   static Library create() {
      Library library;

      for (size_t x = 0; x < COLORS.size(); x++) {
         library
         .add(COLORS[x], Decorator(attr(30 + x), reset))
         .add("bg-" + COLORS[x], Decorator(attr(40 + x), reset));
      }

      library
      .add("bright", Decorator(bright, reset))
      .add("dim", Decorator(dim, reset))
      .add("underscore", Decorator(underscore, reset))
      .add("blink", Decorator(blink, reset))
      .add("reverse", Decorator(reverse, reset))
      .add("hidden", Decorator(hidden, reset))
      .add("fraktur", Decorator(fraktur, reset));

      return library;
   }

   Library& add(const std::string& name, const Decorator& decorator) {
      decorator_map.insert({name, decorator});
      return *this;
   }

   bool has(const std::string& name) {
      return decorator_map.find(name) != decorator_map.end();
   }

   const Decorator operator[](const std::string& name) const {
      auto components = str::split(name, " ");
      auto decorator = none;

      for (auto component : components) {
         auto iter = decorator_map.find(component);
         if (iter == decorator_map.end()) {
            throw core::IndexException("Unknown decorator: \"" + component + "\"");
         } else {
            decorator = decorator.compose(iter->second);
         }
      }

      return decorator;
   }

private:
   DecoratorMap decorator_map;
};

}
}

#endif /* __MOONLIGHT_ANSI_H */
