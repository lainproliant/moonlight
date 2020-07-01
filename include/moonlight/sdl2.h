/*
 * moonlight/sdl2.h: SDL2 specializations and tools.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Friday Feb 6 2015,
 *       Wednesday Jun 19 2019
 */
#ifndef __MOONLIGHT_SDL2_H
#define __MOONLIGHT_SDL2_H

#include "moonlight/time.h"
#include <SDL2/SDL.h>

namespace moonlight {
namespace time {
namespace sdl2 {

typedef Timer<Uint32> Timer;
typedef FrameCalculator<Uint32> FrameCalculator;

inline Timer create_timer(Uint32 interval, bool accumulate = false) {
   return Timer(SDL_GetTicks, interval, accumulate);
}

inline FrameCalculator create_frame_calculator(Timer& timer) {
   return FrameCalculator(create_timer(1000), timer);
}

}
}
}

#endif /* __MOONLIGHT_SDL2_H */
