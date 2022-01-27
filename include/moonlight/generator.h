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
#include <iterator>

namespace moonlight {
namespace gen {

//-------------------------------------------------------------------
// A template allowing the results of a generator lambda, known
// below as "Closure", to be wrapped in a standard library compatible
// iterator type and used with constructs like algorithm templates
// and the ranged-for loop.
//
template<class T>
class Iterator : public std::iterator<std::input_iterator_tag, T> {
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
         THROW(core::UsageError, "Attempted to deference past the end of the sequence.");
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


//-------------------------------------------------------------------
// An object specializing for-each iteration over a type.
// Useful when begin() and end() functions of an object are templates.
//
// Example Usage:
//
// // in api code
// template<class T>
// gen::Iterable<T> iterate() const {
//     return gen::Iterable<T>(gen::begin<T>(...));
// }
//
// // in client code
// for (auto value : variant_vec.iterate<int>()) {
//     ...
// }
//
template<class T>
class Iterable {
public:
    Iterable(const gen::Iterator<T> begin) : _begin(begin) { }
    Iterable(const typename Iterator<T>::Closure& closure) : _begin(closure) { }

    static gen::Iterable<T> empty() {
        return Iterable(end<T>());
    }

    static gen::Iterable<T> singleton(const T& value) {
        bool yielded = false;
        return gen::Iterable<T>([yielded, value]() mutable -> std::optional<T> {
            if (yielded == false) {
                yielded = true;
                return value;
            }
            return {};
        });
    }

    template<class C>
    static gen::Iterable<T> from_collection(const C& coll) {
        static_assert(std::is_same<T, typename C::value_type>(), "Value type must match collection.");
        bool yielded = false;
        return combine([coll, yielded]() mutable -> std::optional<gen::Iterable<T>> {
            if (! yielded) {
                yielded = true;
                return gen::Iterable<T>(gen::wrap(coll.begin(), coll.end()));
            }
            return {};
        });
    }

    static gen::Iterable<T> combine(gen::Iterable<gen::Iterable<T>> stream_of_streams) {
        gen::Iterator<gen::Iterable<T>> stream_iter = stream_of_streams.begin();
        gen::Iterator<T> iter = stream_iter == stream_of_streams.end() ? end<T>() : stream_iter->begin();

        return gen::Iterable<T>([stream_of_streams, stream_iter, iter]() mutable -> std::optional<T> {
            if (stream_iter == stream_of_streams.end()) {
                return {};
            }

            if (iter == stream_iter->end()) {
                stream_iter++;
                if (stream_iter != stream_of_streams.end()) {
                    iter = stream_iter->begin();
                }
            }

            return *iter++;
        });
    }

    static gen::Iterable<T> combine(const std::vector<gen::Iterable<T>>& iterables) {
        return combine(gen::Iterable(wrap(iterables.begin(), iterables.end())));
    }

    gen::Iterator<T> begin() const {
        return _begin;
    }

    gen::Iterator<T> end() const {
        return gen::end<T>();
    }

    gen::Iterable<T> sorted() const {
        std::optional<std::deque<T>> lazy_sorted = {};

        return gen::Iterable<T>([lazy_sorted, this]() mutable -> std::optional<T> {
            if (! lazy_sorted.has_value()) {
                lazy_sorted = std::deque<T>();
                std::copy(this->begin(), this->end(), std::back_inserter(*lazy_sorted));
            }

            if (! lazy_sorted->empty()) {
                auto value = lazy_sorted->front();
                lazy_sorted->pop_front();
                return value;
            } else {
                return {};
            }
        });
    }

    gen::Iterable<T> filter_combine(std::function<gen::Iterable<T>(const T&)> f) const {
        Iterator<T> iter = begin();

        return gen::Iterable<T>([iter, f, this]() mutable -> std::optional<T> {
            return Iterable::combine([iter, f, this] () mutable -> std::optional<Iterable<T>> {
                if (iter == end()) {
                    return {};
                }
                return f(*iter++);
            });
        });
    }

    gen::Iterable<T> filter(std::function<std::optional<T>(const T&)> f) const {
        Iterable<T> iter = begin();

        return gen::Iterable<T>([iter, f, this]() mutable -> std::optional<T> {
            std::optional<T> result;
            while (iter != end()) {
                result = f(*iter++);
                if (result.has_value()) {
                    return result.value();
                }
            }
            return {};
        });
    }

    gen::Iterable<T> unique() const {
        std::set<T> sieve = {};
        Iterator<T> iter = begin();

        return gen::Iterable<T>([sieve, iter, this]() mutable -> std::optional<T> {
            while(iter != end() && sieve.find(*iter) != sieve.end()) {
                iter++;
            }

            if (iter == end()) {
                return {};
            }

            sieve.insert(*iter);
            return *iter++;
        });
    }

    std::vector<T> collect() const {
        std::vector<T> result;
        result.insert(result.begin(), begin(), end());
        return result;
    }

    bool is_empty() const {
        return _begin() == gen::end<T>();
    }

private:
    gen::Iterator<T> _begin;
};


}
}

#endif /* !__MOONLIGHT_GENERATOR_H */
