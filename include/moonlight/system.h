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

#include <cstdlib>
#include <optional>
#include <sstream>
#include <string>

#include "moonlight/exceptions.h"

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

//-------------------------------------------------------------------
// Check the output of a command and return it as a string.
//
// If the command does not execute or complete successfully,
// a core::RuntimeError is thrown.
//
inline std::string check(const std::string& command) {

    int returncode;
    std::string output;

    if (! debug::check(command, output, returncode)) {
#ifdef MOONLIGHT_SYS_HIDE_COMMANDS
        THROW(core::RuntimeError, "Command could not be started.");
#else
        THROW(core::RuntimeError, "Command \"" + command + "\" could not be started.");

#endif
    }

    if (returncode != 0) {
#ifdef MOONLIGHT_SYS_HIDE_COMMANDS
        THROW(core::RuntimeError, "Command failed with exit code " + std::to_string(returncode));
#else
        THROW(core::RuntimeError, "Command \"" + command + "\" failed with exit code " + std::to_string(returncode));
#endif
    }

    return output;
}

}

#endif /* !__MOONLIGHT_SYSTEM_H */
