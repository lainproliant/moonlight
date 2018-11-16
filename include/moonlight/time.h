/*
 * time.h: An abstract timer and relative timer wrapper.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Friday, November 9 2018
 *
 * Based on legacy Timer code written in 2011-2014 by Lee Supe.
 */
#pragma once
#include "moonlight/core.h"

namespace moonlight {
namespace time {
using namespace std;

template<class T>
class RelativeTimer;

//-------------------------------------------------------------------
template <class T>
class Timer : public enable_shared_from_this<Timer<T>> {
public:
   typedef function<T(void)> TimeFunction;
   static shared_ptr<Timer<T>> create(TimeFunction time_function, T interval,
                                      bool accumulate = false) {
      return shared_ptr<Timer<T>>(new Timer<T>(time_function, interval, accumulate));
   }

   virtual ~Timer() { }

   T frames() const {
      return frames;
   }

   T time() const {
      return t1;
   }

   T wait_time() const {
      T tnow = time_function();

      if (tnow < t0 || tnow >= t0 + interval) {
         return 0;

      } else {
         return t0 + interval - tnow;
      }
   }

   shared_ptr<Timer<T>> relative_timer(T frameInterval = 0,
                                       bool accumulate = false) const {
      return make_shared<RelativeTimer<T>>(
          this->shared_from_this(), frameInterval, accumulate);
   }

   virtual shared_ptr<Timer<T>> copy() const {
      return shared_ptr<Timer<T>>(new Timer(*this));
   }

   void pause() {
      paused = true;
   }

   void reset() {
      t0 = time_function();
      t1 = t0;
      t2 = t0 + interval;
      frames_ = 0;
   }

   void set_interval(const T& newInterval) {
      interval = newInterval;
      reset();
   }

   void start() {
      T tnow = time_function();
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

      T tnow = time_function();
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
   Timer(TimeFunction time_function, T interval, bool accumulate = false) :
   time_function(time_function), interval(interval), accumulate(accumulate) {
      t0 = 0;
      t1 = 0;
      tacc = 0;
      frames_ = 0;
      paused = true;
   }

private:
   bool paused, accumulate;
   T t0, t1, t2, tacc, interval, frames_;
   TimeFunction time_function;
};

template<class T>
class RelativeTimer : public Timer<T> {
public:
   RelativeTimer(const shared_ptr<const Timer<T>>& referenceTimer, T interval, bool accumulate = false) :
   Timer<T>([referenceTimer]() -> T {
      return referenceTimer->frames();
   }, interval, accumulate) { }
   virtual ~RelativeTimer() { }

   shared_ptr<Timer<T>> copy() const override {
      return shared_ptr<Timer<T>>(new RelativeTimer(*this));
   }
};

/**
 * Calculate and report FPS metrics.
 *
 * USAGE:
 * - Construct the FrameCalculator with a timer that fires every second
 *   (the monitor_timer) and a timer to be monitored (the monitoring_timer).
 * - Call update() regularly to update the FPS metric.
 * - Call fps() to get the FPS metric.  If the FPS metric has not yet
 *   been calculated, the number of frames passed on the monitoring_timer
 *   will be returned.
 */
template<class T>
class FrameCalculator {
public:
   FrameCalculator(shared_ptr<Timer<T>> monitor_timer,
                   shared_ptr<const Timer<T>> monitoring_timer) :
   monitor_timer(monitor_timer), monitoring_timer(monitoring_timer) {
      monitor_timer->start();
   }

   void update() {
      if (monitor_timer->update()) {
         T frames = monitoring_timer->frames();
         fps_ = frames - prev_frames;
         fps_calculated = true;
         prev_frames = frames;
      }
   }

   T fps() const {
      if (! fps_calculated) {
         return monitoring_timer->frames();

      } else {
         return fps;
      }
   }

private:
   shared_ptr<Timer<T>> monitor_timer;
   shared_ptr<const Timer<T>> monitoring_timer;

   T fps_ = 0;
   T prev_frames = 0;
   bool fps_calculated = false;
};
}
}
