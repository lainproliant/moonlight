/*
 * ## posix.h: POSIX specializations and tools. -------------------------------
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

inline uint64_t get_ticks() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec * 1000 + (tp.tv_nsec / 1000000UL);
}

typedef Timer<uint64_t> Timer;
typedef FrameCalculator<uint64_t> FrameCalculator;

inline std::shared_ptr<Timer> create_timer(uint64_t interval, bool accumulate = false) {
    return Timer::create(get_ticks, interval, accumulate);
}

inline std::shared_ptr<FrameCalculator> create_frame_calculator(std::shared_ptr<Timer> timer) {
    return FrameCalculator::create(create_timer(1000), timer);
}

}  // namespace posix
}  // namespace time
}  // namespace moonlight

#endif /* __MOONLIGHT_POSIX_H */
