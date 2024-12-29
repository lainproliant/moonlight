/*
 * ## sdl3.h: SDL3 specializations and tools. -----------------------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday November 28, 2024
 */

#ifndef __MOONLIGHT_SDL3_H
#define __MOONLIGHT_SDL3_H

#include <SDL3/SDL.h>
#include "moonlight/time.h"

namespace moonlight {
namespace sdl3 {

typedef time::Timer<Uint64> Timer;
typedef time::FrameCalculator<Uint64> FrameCalculator;

inline std::shared_ptr<Timer> create_timer_ms(Uint64 interval, bool accumulate = false) {
    return Timer::create(SDL_GetTicks, interval, accumulate);
}

inline std::shared_ptr<Timer> create_timer_ns(Uint64 interval, bool accumulate = false) {
    return Timer::create(SDL_GetTicksNS, interval, accumulate);
}

}


#endif /* !__MOONLIGHT_SDL3_H */
