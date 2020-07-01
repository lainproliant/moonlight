/*
 * moonlight/debug.h: Functions and macros for debugging purposes.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Friday, July 12 2019
 */
#ifndef __MOONLIGHT_DEBUG_H
#define __MOONLIGHT_DEBUG_H

//-------------------------------------------------------------------
#ifdef MOONLIGHT_DEBUG
#include <csignal>
#define debugger raise(SIGSEGV)
#else
#define debugger (void)0
#endif

//-------------------------------------------------------------------
#ifdef MOONLIGHT_DEBUG
#include <tinyformat/tinyformat.h>
template<typename... Args>
inline void dbgprint(const std::string& fmt, const Args&... args) {
   tfm::format(std::cerr, fmt, args...);
}

#else
#define dbgprint(...) (void)0

#endif

//-------------------------------------------------------------------
#ifdef MOONLIGHT_DEBUG
#define moonlight_assert(x) \
   if (! (x)) { \
      throw moonlight::core::AssertionFailure(#x, __FUNCTION__, __FILE__, __LINE__); \
   }
#else
#define moonlight_assert(x) (void)0;
#endif

#endif /*__MOONLIGHT_DEBUG_H */
