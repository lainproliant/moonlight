/*
 * moonlight/posix.h: POSIX specializations and tools.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Wednesday Jun 19 2019
 */
#ifndef __MOONLIGHT_POSIX_H
#define __MOONLIGHT_POSIX_H

#include "moonlight/time.h"

namespace moonlight {
namespace time {
namespace posix {

inline unsigned long get_ticks() {
   struct timespec tp;
   clock_gettime(CLOCK_MONOTONIC, &tp);
   return tp.tv_sec * 1000 + (tp.tv_nsec / 1000000UL);
}

typedef Timer<unsigned long> Timer;
typedef FrameCalculator<unsigned long> FrameCalculator;

inline Timer create_timer(unsigned long interval, bool accumulate = false) {
   return Timer(get_ticks, interval, accumulate);
}

inline FrameCalculator create_frame_calculator(Timer& timer) {
   return FrameCalculator(create_timer(1000), timer);
}

}
}
}

#endif /* __MOONLIGHT_POSIX_H */
