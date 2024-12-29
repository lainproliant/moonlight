/*
 * ## sdl2.h: SDL2 specializations and tools. -----------------------
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Friday Feb 6 2015,
 *       Wednesday Jun 19 2019
 */
#ifndef __MOONLIGHT_SDL2_H
#define __MOONLIGHT_SDL2_H

#include <SDL2/SDL.h>
#include "moonlight/time.h"

namespace moonlight {
namespace time {
namespace sdl2 {

typedef Timer<Uint32> Timer;
typedef FrameCalculator<Uint32> FrameCalculator;

inline std::shared_ptr<Timer> create_timer(Uint32 interval, bool accumulate = false) {
    return Timer::create(SDL_GetTicks, interval, accumulate);
}

inline std::shared_ptr<FrameCalculator> create_frame_calculator(std::shared_ptr<Timer> timer) {
    return FrameCalculator::create(create_timer(1000), timer);
}

}  // namespace sdl2
}  // namespace time
}  // namespace moonlight

#endif /* __MOONLIGHT_SDL2_H */
