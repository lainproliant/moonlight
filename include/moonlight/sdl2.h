/*
 * moonlight/sdl2.h: SDL2 specializations and tools.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Friday Feb 6 2015,
 *       Wednesday Jun 19 2019
 */
#pragma once
#include <SDL2/SDL.h>
#include "moonlight/time.h"

namespace moonlight {
namespace time {
namespace sdl2 {

typedef std::shared_ptr<Timer<uint32_t>> TimerPointer;
typedef std::shared_ptr<FrameCalculator<uint32_t>> FrameCalculatorPointer;

inline TimerPointer create_timer(uint32_t interval, bool accumulate = false) {
   return Timer<uint32_t>::create(SDL_GetTicks, interval, accumulate);
}

inline FrameCalculatorPointer create_frame_calculator(TimerPointer timer) {
   return std::make_shared<FrameCalculator<uint32_t>>(create_timer(1000), timer);
}

}
}
}
