/*
 * ## time.h: A timer and relative time wrappers. -------------------
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Sunday Jan 25 2014,
 *       Wednesday Jun 19 2019
 *
 * ## Usage ---------------------------------------------------------
 * This header provides an optionally accumulating timer which can be defined in
 * terms of any function which provides a numeric time value that increases with
 * time.  The following headers in `moonlight` provide pre-defined
 * specializations of `Timer`:
 *
 * - `posix.h`: Defines `posix::Timer`, which is based on `clock_gettime(2)` and
 *   the monotonic system clock.`
 * - `sdl2.h`: Defines `sdl2::Timer`, defined by `SDL_GetTicks`.
 * - `sdl3.h`: Defines `sdl3::Timer`, defined by `SDL_GetTicks64`.
 *
 * The `Timer` class when constructed is provided with an interval, espressed in
 * the base units of the numeric time function.  The timer is said to "tick"
 * each time this interval elapses.  First, call `start()` to unpause the timer.
 * Then, the typical usage pattern is to call `update()` on the timer as
 * frequently as possible, which will return `false` if a tick has not yet
 * elapsed and `true` if a tick has elapsed.  You can later pause the timer
 * again by calling `pause()`.  Calls to `update()` will not yield ticks while
 * the timer is paused.  You can also call the `wait_time()` method to determine
 * how many base units of time remain before the next tick will elapse, which
 * can inform how long you should sleep to save CPU cycles.
 *
 * If `accumulate` is set to `true` on Timer creation, the timer will keep track
 * of how long it actually took for the `update()` method to be called to
 * reflect each tick, and add that to an accumulator which will deduct that
 * amount of time from the time that must elapse before the next tick is
 * registered.
 *
 * Timers can be shared using an `std::shared_ptr`.  The `Timer` class offers
 * `copy()`, which will create an `std::shared_ptr` copy of the timer which will
 * operate separately from the original timer.  It also offers
 * `relative_timer()`, which will create an new timer who's base units of time
 * are defined in terms of the ticks of this timer.
 */
#ifndef __MOONLIGHT_TIME_H
#define __MOONLIGHT_TIME_H

#include <memory>
#include <functional>

namespace moonlight {
namespace time {

template<class T>
class RelativeTimer;

template <class T>
class Timer : public std::enable_shared_from_this<Timer<T>> {
 public:
     typedef std::function<T(void)> TimeFunction;

     Timer(TimeFunction getTime, T interval, bool accumulate = false) :
     interval(interval), accumulate(accumulate), getTime(getTime) {
         t0 = 0;
         t1 = 0;
         tacc = 0;
         frames_ = 0;
         paused = true;
     }

     virtual ~Timer() { }

     static std::shared_ptr<Timer<T>> create(TimeFunction getTime, T interval, bool accumulate = false) {
         return std::make_shared<Timer>(getTime, interval, accumulate);
     }

     T frames() const {
         return frames_;
     }

     T time() const {
         return t1;
     }

     T wait_time() const {
         T tnow = getTime();

         if (tnow < t0 || tnow >= t0 + interval) {
             return 0;

         } else {
             return t0 + interval - tnow;
         }
     }

     std::shared_ptr<Timer<T>> relative_timer(T frameInterval = 0, bool accumulate = false) {
         return make_shared<RelativeTimer<T>>(this->shared_from_this(),
                                              frameInterval, accumulate);
     }

     virtual std::shared_ptr<Timer<T>> copy() {
         return std::shared_ptr<Timer<T>>(new Timer(*this));
     }

     void pause() {
         paused = true;
     }

     void reset() {
         t0 = getTime();
         t1 = t0;
         t2 = t0 + interval;
         frames_ = 0;
     }

     void set_interval(const T& newInterval) {
         interval = newInterval;
         reset();
     }

     void start() {
         T tnow = getTime();
         T tdiff = t1 - t0;

         t0 = tnow;
         t1 = tnow + tdiff;
         t2 = t0 + interval;
         paused = false;
     }

     bool update(T* pterr = nullptr) {
         if (paused || interval == 0) {
             return false;
         }

         T tnow = getTime();
         if (tnow < t0) {
             // The timer function has wrapped.
             T tdiff = t1 - t0;
             t0 = tnow;
             t1 = tnow + tdiff;
             t2 = t0 + interval;
             return update();
         }

         t1 = tnow;
         if (t1 >= t2) {
             T terr = t1 - t2;

             t0 = tnow;
             t1 = t0;
             frames_ ++;

             if (accumulate) {
                 // Subtract a frame from the accumulator.
                 if (tacc > interval) {
                     tacc -= interval;
                 } else {
                     tacc = 0;
                 }

                 tacc += terr;

                 if (tacc > interval) {
                     t2 = t1;

                 } else {
                     t2 = t0 + (interval - tacc);
                 }

             } else {
                 t2 = t0 + interval;
             }

             if (pterr != nullptr) {
                 *pterr = terr;
             }

             return true;

         } else {
             return false;
         }
     }

 protected:
     T interval;
     bool accumulate;

 private:
     bool paused;
     T t0, t1, t2, tacc, frames_;
     TimeFunction getTime;
};

template<class T>
class RelativeTimer : public Timer<T> {
 public:
     RelativeTimer(std::shared_ptr<Timer<T>> reference, T interval, bool accumulate = false) :
     Timer<T>([reference]() -> T {
         return reference->frames();
     }, interval, accumulate),
     reference(reference) { }
     virtual ~RelativeTimer() { }

     std::shared_ptr<Timer<T>> copy() override {
         return std::make_shared<RelativeTimer<T>>(reference, this->interval,
                                                   this->accumulate);
     }

 private:
     const std::shared_ptr<Timer<T>> reference;
};

/**
 * FrameCalculator <concrete class>
 *
 * Calculate and report FPS metrics.
 *
 * USAGE:
 * - Construct the FrameCalculator with a timer that fires every second
 *   (the monitor_timer) and a timer to be monitored (the monitoring_timer).
 * - Call update() regularly to update the FPS metric.
 * - Call get_fps() to get the FPS metric.  If the FPS metric has not yet
 *   been calculated, the number of frames passed on the monitoring_timer
 *   will be returned.
 */
template<class T>
class FrameCalculator {
 public:
     FrameCalculator(std::shared_ptr<Timer<T>> monitor_timer,
                     std::shared_ptr<Timer<T>> monitoring_timer) :
     monitor_timer(monitor_timer),
     monitoring_timer(monitoring_timer) {
         this->monitor_timer->start();
     }

     static std::shared_ptr<FrameCalculator> create(std::shared_ptr<Timer<T>> monitor_timer, std::shared_ptr<Timer<T>> monitoring_timer) {
         return std::make_shared<FrameCalculator>(monitor_timer, monitoring_timer);
     }

     void update() {
         if (monitor_timer->update()) {
             T frames = monitoring_timer->frames();
             fps = frames - prev_frames;
             prev_frames = frames;
         }
     }

     T get_fps() const {
         if (fps == 0) {
             return monitoring_timer->frames();

         } else {
             return fps;
         }
     }

 private:

     std::shared_ptr<Timer<T>> monitor_timer;
     std::shared_ptr<Timer<T>> monitoring_timer;

     T fps = 0;
     T prev_frames = 0;
};

}  // namespace time
}  // namespace moonlight
#endif /* __MOONLIGHT_TIME_H */
