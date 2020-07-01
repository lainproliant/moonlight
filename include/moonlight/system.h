/*
 * system.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_SYSTEM_H
#define __MOONLIGHT_SYSTEM_H

#include <string>
#include <optional>
#include <cstdlib>

namespace moonlight {
namespace sys {

//-------------------------------------------------------------------
inline std::optional<std::string> getenv(const std::string& name) {
   const char* value = std::getenv(name.c_str());
   if (value != nullptr) {
      return value;
   } else {
      return {};
   }
}
}

}



#endif /* !__MOONLIGHT_SYSTEM_H */
