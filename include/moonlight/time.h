/*
 * moonlight/time.h: A timer and relative time wrappers.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Sunday Jan 25 2014,
 *       Wednesday Jun 19 2019
 */
#ifndef __MOONLIGHT_TIME_H
#define __MOONLIGHT_TIME_H

#include <memory>
#include <algorithm>
#include <functional>

namespace moonlight {
namespace time {
   using namespace std;

   template<class T>
   class RelativeTimer;

   template <class T>
   class Timer {
   public:
      typedef function<T(void)> TimeFunction;

      Timer(TimeFunction getTime, T interval, bool accumulate = false) :
         getTime(getTime), interval(interval), accumulate(accumulate) {
         t0 = 0;
         t1 = 0;
         tacc = 0;
         frames_ = 0;
         paused = true;
      }


      virtual ~Timer() { }

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
      RelativeTimer(const Timer<T>& reference, T interval, bool accumulate = false) :
         Timer<T>([reference]() -> T {
                     return reference->frames();
                  }, interval, accumulate),
      reference(reference) { }
      virtual ~RelativeTimer() { }

      RelativeTimer<T> copy() const override {
         return RelativeTimer<T>(reference, this->interval,
                                 this->accumulate);
      }

   private:
      const Timer<T>& reference;
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
      FrameCalculator(const Timer<T>& monitor_timer,
                      const Timer<T>& monitoring_timer) :
         monitor_timer(monitor_timer),
         monitoring_timer(monitoring_timer) {
         this->monitor_timer.start();
      }

      void update() {
         if (monitor_timer.update()) {
            T frames = monitoring_timer.frames();
            fps = frames - prev_frames;
            prev_frames = frames;
         }
      }

      T get_fps() const {
         if (fps == 0) {
            return monitoring_timer.frames();

         } else {
            return fps;
         }
      }

   private:
      Timer<T> monitor_timer;
      const Timer<T>& monitoring_timer;

      T fps = 0;
      T prev_frames = 0;
   };
}
}
#endif /* __MOONLIGHT_TIME_H */
