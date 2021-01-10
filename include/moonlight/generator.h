/*
 * generator.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_GENERATOR_H
#define __MOONLIGHT_GENERATOR_H

#include "moonlight/automata.h"

#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>
#include <optional>

namespace moonlight {
namespace gen {

//-------------------------------------------------------------------
class Error : public core::Exception {
   using Exception::Exception;
};

//-------------------------------------------------------------------
// A template allowing the results of a generator lambda, known
// below as "Closure", to be wrapped in a standard library compatible
// iterator type and used with constructs like algorithm templates
// and the ranged-for loop.
//
template<class T>
class Iterator {
public:
   typedef std::function<std::optional<T>()> Closure;

   Iterator() : _value({}), _position(-1) { }
   Iterator(const Closure& closure) : _closure(closure), _value(_closure()) { }

   Iterator<T>& operator++() {
      if (_value.has_value()) {
         auto new_value = _closure();
         if (new_value.has_value()) {
            _value.emplace(new_value.value());
            _position ++;
         } else {
            _value.reset();
            _position = -1;
         }
      }
      return *this;
   }

   Iterator<T> operator++(int) {
      Iterator iter = *this;
      ++(*this);
      return iter;
   }

   bool operator==(const Iterator<T>& other) {
      return (_position == -1 && other._position == -1) ||
         (&_closure == &other._closure && _position == other._position);
   }

   bool operator==(Iterator<T>& other) {
      return (_position == -1 && other._position == -1) ||
         (&_closure == &other._closure && _position == other._position);
   }

   bool operator!=(const Iterator<T>& other) {
      return !(*this == other);
   }

   bool operator!=(Iterator<T>& other) {
      return !(*this == other);
   }

   const T& operator*() {
      if (_value.has_value()) {
         return *_value;
      } else {
         throw Error("Attempted to deference past the end of the sequence.");
      }
   }

   using value_type = T;
   using pointer = const T*;
   using reference = const T&;
   using iterator_category = std::forward_iterator_tag;

private:
   Closure _closure;
   int _position = 0;
   std::optional<T> _value;
};

//-------------------------------------------------------------------
// A queue template for asynchronously communicating the output
// of a generator running in another thread with one or more
// consumer threads.
//
template<class T>
class Queue {
public:
   typedef std::future<std::optional<T>> Future;
   typedef std::function<void(const T&)> Handler;

   // Indicates that the generator has no more results to provide.
   void complete() {
      std::unique_lock<std::mutex> lock(_mutex);
      _completed = true;
      lock.unlock();
      _cv.notify_all();
   }

   // Called by the generator to yield a result into the queue.
   void yield(const T& value) {
      std::unique_lock<std::mutex> lock(_mutex);
      _yielded_items.push_back(value);
      lock.unlock();
      _cv.notify_one();
   }

   // Called by the consumer to get the next result from the queue.
   // Blocks until a result is provided, and may return an empty
   // optional if the generator completes after this method is
   // called on another thread and there are no more results to
   // consume.
   std::optional<T> next() {
      std::unique_lock<std::mutex> lock(_mutex);
      _cv.wait(lock, [&]{ return _yielded_items.empty() || _completed; });

      if (_yielded_items.empty() && _completed) {
         return {};
      }

      T value = _yielded_items.front();
      _yielded_items.pop_front();
      return value;
   }

   // See above, simply wraps the blocking behavior of `next()`
   // in an async future.
   Future async_next() {
      return std::async(std::launch::async, [&] {
         return this->next();
      });
   }

   // Block until all values from the generator have been consumed
   // and handled by the given handler callback.
   void process(Handler handler) {
      std::optional<T> value;
      while (value = this->next(), value) {
         handler(*value);
      }
   }

   // Wraps the above `process()` behavior in an async future.
   std::future<void> process_async(Handler handler) {
      return std::async(std::launch::async, [&] {
         std::optional<T> value;
         while (value = this->next(), value) {
            handler(*value);
         }
      });
   }

private:
   std::deque<T> _yielded_items;
   std::mutex _mutex;
   std::condition_variable _cv;
   bool _completed = false;
};

//-------------------------------------------------------------------
// Return the beginning of a virtual range representing the values
// yielded by the given generator lambda.
template<class T>
Iterator<T> begin(std::function<std::optional<T>()> lambda) {
   return Iterator<T>(lambda);
}

//-------------------------------------------------------------------
// Return the end of a virtual range, any virtual range, of a type.
template<class T>
Iterator<T> end() {
   return Iterator<T>();
}

//-------------------------------------------------------------------
// Returns a queue for processing the results of a generator
// asynchronously.  The first parameter is a generator factory,
// any subsequent parameters are forwarded to this factory to produce
// the generator.
//
template<class T, class... TD>
std::shared_ptr<Queue<T>> async(std::function<std::optional<T>(TD...)> factory, TD... params) {

   std::shared_ptr<Queue<T>> queue = std::make_shared<Queue<T>>();
   std::async(std::launch::async, [&] {
      for (auto iter = begin(factory, std::forward(params)...);
           iter != end<T>();
           iter++) {
         queue->yield(*iter);
      }
      queue->complete();
   });
   return queue;
}

//-------------------------------------------------------------------
// Wraps the given set of iterators as a range into a generator.
//
template<class I>
std::function<std::optional<typename I::value_type>()> iterate(I begin, I end) {
    I current = begin;
    return [=]() mutable -> std::optional<typename I::value_type> {
        if (current == end) {
            return {};
        } else {
            return *(current++);
        }
    };
}

//-------------------------------------------------------------------
// Wraps the given set of iterators as a range into a generator,
// then returns the beginning of a virtual range wrapping that
// generator.  Use this to reinterpret iterators of any type
// into virtual iterators.
//
template<class I>
Iterator<typename I::value_type> wrap(I begin_in, I end_in) {
    if (begin_in == end_in) {
        return Iterator<typename I::value_type>();
    }
    return begin<typename I::value_type>(iterate(begin_in, end_in));
}

}
}

#endif /* !__MOONLIGHT_GENERATOR_H */
