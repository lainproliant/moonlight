/*
 * moonlight/posix.h: POSIX specializations and tools.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Wednesday Jun 19 2019
 */
#pragma once
#include <SDL2/SDL.h>
#include "moonlight/time.h"

namespace moonlight {
namespace time {
namespace posix {

inline unsigned long get_ticks() {
   struct timespec tp;
   clock_gettime(CLOCK_MONOTONIC, &tp);
   return tp.tv_sec + (tp.tv_nsec / 1000000UL);
}

inline std::shared_ptr<Timer<unsigned long>> create_timer(
    unsigned long interval, bool accumulate = false) {
   return Timer<unsigned long>::create(get_ticks, interval, accumulate);
}

inline std::shared_ptr<FrameCalculator<unsigned long>> create_frame_calculator(
    std::shared_ptr<const Timer<unsigned long>> timer) {
   return std::make_shared<FrameCalculator<unsigned long>>(create_timer(1000), timer);
}

}
}
}
