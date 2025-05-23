/*
 * ## generator.h: Implements generators like those in Python. ------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 26, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library implements generators similar to the ones offered in the Python
 * programming language using a combination of iterator wrappers and mutable
 * closures acting as generator functions.  The following classes and utility
 * function templates are provided.
 *
 * - `gen::Generator<T>`: An alias for `std::function<std::optional<T>>`
 *   representing a closure or function object which returns values of type `T`
 *   until no more values are available.
 * - `gen::Iterator<T>`: Wraps a `gen::Generator<T>` in a class that behaves
 *   like an STL-compatible forward iterator, allowing them to be used with
 *   `<algorithm>` templates and the ranged-for loop.
 * - `gen::Stream<T>`: An iterator wrapper for functional composition over
 *   items in a virtual range.
 * - `gen::Queue<T>`: A queue template for async communication of output from a
 *   generator running in another thread to one or more consumer threads.
 * - `gen::Buffer<T>`: An alias for `std::deque<T>` used to buffer streams.
 * - `gen::begin<T>(g)`: Creates a `gen::Iterator` for the given generator `g`.
 * - `gen::end<T>()`: Represents the end of any virtual range of type `T`.
 * - `gen::async(f, ...)`: Creates a `gen::Queue` for processing the results
 *   of a generator in another thread and communicating the output to one or
 *   more consumer threads.
 * - `gen::iterate(begin, end)`: Wraps a given pair of STL-compatible `begin`
 *   and `end` iterators as a virtual range by creating a generator which yields
 *   the items between `begin` and `end`.
 * - `gen::wrap(begin, end)`: Wraps the given pair of STL-compatible `begin` and
 *   `end` iterators into a `gen::Iterator`.
 * - `gen::stream(v)`: Creates a `gen::Stream` from the given value `v`, which
 *   may be a generator or a collection.
 * - `gen::one(v)`: Alias for `Stream::singleton(v)`.
 * - `gen::nothing()`: Alias for `Stream::empty()`.
 *
 * The class `gen::Stream<T>` facilitates high-order functional composition of
 * generators, streams, and collections.  This class wraps `gen::Iterator<T>`
 * and provides functional operations similar to those in `java.util.Stream` in
 * Java.  Streams support the `+` and `+=` operators for logical concatenation.
 * The following methods are available on `gen::Stream`:
 *
 * - `Stream::singleton(v)`: Creates a stream consisting of only one value.
 * - `Stream::lazy_singleton(f)`: Creates a lazy single element stream from the
 *   given factory function.  The value of the single object in the stream will
 *   not be determined until the stream is accessed.
 * - `Stream::lazy(f)`: Generates a lazy stream from the given factory function, which
 *   won't be created until it is accessed.
 * - `Stream<T>::empty()`: Creates an empty stream.
 * - `advance(n)`: Advance the stream forward `n` items which are skipped.
 * - `begin()`: The beginning of the stream's virtual range.
 * - `end()`: The end of the stream's virtual range.
 * - `trim_left(n)`: Trim `n` items off of the left of the virtual range.
 * - `trim_right(n)`: Trim `n` items off of the right of the virtual range.
 *   This requires the creation of an arbitrarily large buffer, since the size
 *   of the virtual range is unknown until its end is reached through iteration.
 * - `trim(left, right)`: Trim the given amount off of the left and right sides
 *   of the virtual range.  This may require the creation of an arbitrarily
 *   large buffer if `right` is non-zero, since the size of the virtual range is
 *   unknown until its end is reached through iteration.
 * - `buffer(bufsize, squash?)`: Buffer the stream `bufsize` items forward.  The
 *   resulting stream elements are all references to a `gen::Buffer`.  If
 *   `squash?` is true, items are removed from the beginning of the buffer each
 *   step until it is empty and the buffer may be smaller than `bufsize`.
 *   Otherwise and by default, the buffered sequence ends when the scalar
 *   sequence ends and the buffer is not allowed to be smaller than `bufsize`
 *   unless the stream's virtual range happens to be.
 * - `last()`: Scans to the very last item in the stream and returns it.
 * - `limit(n)`: Create a new stream which will only emit the next `n` elements
 *   of this stream.
 * - `sorted()`: Sorts all of the elements in the stream, then streams from the
 *   resulting sorted collection.  This requires the full virtual range to
 *   be buffered in memory.
 * - `transform_split<R=T>(f)`: Streams a new stream-of-streams resulting from
 *   applying `f` to each element in this stream, where `f` generates a new
 *   stream of results for each element in this stream.  The template parameter
 *   `R` may be provided or implied, allowing the stream to be transformed into
 *   a stream-of-streams of a different type.
 * - `transform<R=T>(f)`: Transform each element in the stream using the given
 *   function, which must return an optional of the stream type.  Items for
 *   which the function returns an empty optional are not included in the new
 *   stream, which can be used to filter the stream while transforming.
 *   The template parameter `R` may be provided or implied, allowing the stream
 *   to be transformed into a stream of a different type.
 * - `for_each(f)`: Call the given function for each element in the stream.  `f`
 *   may optionally be a function returning a `bool`, which should return `true`
 *   to indicate that the loop should continue and `false` to indicate that it
 *   should be terminated early.
 * - `filter(f)`: A wrapper around `transform<T>()` using `f` as a predicate
 *   function to indicate which items should be filtered from the resulting
 *   stream.
 * - `unique(f)`: Filters the stream into a new stream containing only unique items
 *   using an `std::set<T>` as a seive.
 * - `collect<C=std::vector>()`: Collects all of the items in the stream into an collection
 *   of the given type `C`.
 * - `map_collect<M>(f)`: Collects all of the items in the stream into a map of
 *   type `M` using the given mapping function `f` to determine the resulting key
 *   and mapped value.
 * - `all(f)`: Determine if all of the items in the collection match a given
 *   predicate.  This will exhaust the stream until an item which does not match
 *   the predicate is found.
 * - `any(f)`: Determine if any of the items in the stream match a predicate.
 *   This will exhaust the stream until an item which matches the predicate is
 *   found.
 * - `none(f)`: Determine if none of the items in the stream match a predicate.
 *   This will exhaust the stream until an item which does not match the
 *   predicate is found.
 * - `reduce<R=T>(f, init=R())`: Reduce the items in the stream into a single
 *   value using the given reducer function and initial value, which is default
 *   constructed from the result type by default.
 * - `sum<R=T>()`: Calculates a sum from all of the items in the stream.  The
 *   result type may be a scalar or any other type.  Also useful for
 *   concatenating a stream-of-streams into a single stream.
 * - `join(sep)`: Joins all of the items in the stream together as a single
 *   string with the given separator.
 * - `is_empty()`: Determine if the stream is currently empty.
 */
#ifndef __MOONLIGHT_GENERATOR_H
#define __MOONLIGHT_GENERATOR_H

#include <deque>
#include <mutex>
#include <condition_variable>
#include <future>
#include <optional>
#include <memory>
#include <set>
#include <utility>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>

#include "moonlight/exceptions.h"

namespace moonlight {
namespace gen {

template<class T>
using Generator = std::function<std::optional<T>()>;

/* ------------------------------------------------------------------
 * A template class allowing the results of a Generator lambda to be
 * wrapped in a standard library compatible iterator type and used with
 * constructs like algorithm templates and the ranged-for loop.
 *
 * Used as a building block for higher level composition abstractions,
 * such as gen::Stream.
 */
template<class T>
class Iterator {
 public:
     typedef Generator<T> Closure;

     using iterator_category = std::input_iterator_tag;
     using difference_type = ptrdiff_t;
     using value_type = T;
     using pointer = const T*;
     using reference = const T&;

     Iterator() : _position(-1), _value({}) { }
     Iterator(const Closure& closure) : _closure(closure), _value(_closure()) {
         if (! _value.has_value()) {
             _position = -1;
         }
     }
     Iterator(const Iterator& iter)
     : _closure(iter._closure), _position(iter._position), _value(iter._value) { }

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
         Iterator iter(*this);
         ++(*this);
         return iter;
     }

     Iterator<T> advance(const int distance) {
         auto iter = *this;
         for (int x = 0; x < distance && iter != gen::Iterator<T>(); x++, iter++) { }
         return iter;
     }

     bool operator==(const Iterator<T>& other) const {
         return (_position == -1 && other._position == -1) ||
         (&_closure == &other._closure && _position == other._position);
     }

     bool operator!=(const Iterator<T>& other) const {
         return !(*this == other);
     }

     int position() const {
         return _position;
     }

     const T& operator*() {
         if (_value.has_value()) {
             return *_value;
         } else {
             THROW(core::UsageError, "Attempted to dereference past the end of the sequence.");
         }
     }

     const T* operator->() {
         return &(operator*());
     }

 private:
     Closure _closure;
     int _position = 0;
     std::optional<T> _value;
};

/* ------------------------------------------------------------------
 * A queue template for asynchronously communicating the output
 * of a generator running in another thread with one or more
 * consumer threads.
 */
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
         return std::async(std::launch::async, [=,this] {
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

/**
 * Type used to buffer stream results for look-ahead.
 */
template<class T>
using Buffer = std::deque<T>;

/**
 * Return the beginning of a virtual range representing the values
 * yielded by the given Generator lambda.
 */
template<class T>
Iterator<T> begin(Generator<T> lambda) {
    return Iterator<T>(lambda);
}

/**
 * Return the end of any virtual range of the given type.
 */
template<class T>
Iterator<T> end() {
    return Iterator<T>();
}

/**
 * Creates a queue for processing the results of a Generator
 * asynchronously.
 *
 * @param factory A generator factory.
 * @param ... Subsequent parameters forwarded to the generator factory.
 * @return a shared_ptr to a newly created Queue object.
 */
template<class T, class... TD>
std::shared_ptr<Queue<T>> async(std::function<std::optional<T>(TD...)> factory, TD... params) {
    std::shared_ptr<Queue<T>> queue = std::make_shared<Queue<T>>();
    auto thread = std::async(std::launch::async, [queue, factory, params...] {
        for (auto iter = begin(factory, std::forward(params)...);
             iter != end<T>();
             iter++) {
            queue->yield(*iter);
        }
        queue->complete();
        return 0;
    });
    return queue;
}

/**
 * Wraps the given set of iterators as a range into a Generator.
 */
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

/**
 * Wraps the given set of iterators as a range into a Generator,
 * then returns the beginning of the virtual range wrapping that
 * Generator.  Use this to reinterpret iterators of any type
 * as virtual Iterators.
 */
template<class I>
Iterator<typename I::value_type> wrap(I begin_in, I end_in) {
    if (begin_in == end_in) {
        return Iterator<typename I::value_type>();
    }
    return begin<typename I::value_type>(iterate(begin_in, end_in));
}

/**
 * An iterator wrapper for functional composition over items in a virtual range.
 */
template<class T>
class Stream {
 public:
     Stream() : _begin(gen::end<T>()) { }
     Stream(const gen::Iterator<T>& begin) : _begin(begin) { }
     Stream(const Generator<T>& generator) : _begin(generator) { }
     Stream(const Stream<T>& stream) : _begin(stream._begin) { }

     typedef T value_type;

     typedef std::function<Stream<T>()> Factory;

     /**
      * Lazily generates a stream from the given Factory function.
      */
     static gen::Stream<T> lazy(const Factory& f);

     /**
      * Creates a stream consisting of one value.
      */
     static gen::Stream<T> singleton(const T& value);

     /**
      * Creates a stream consisting of a single value result of a nullary
      * function call, which isn't evaluated until the stream is read.
      */
     static gen::Stream<T> lazy_singleton(std::function<T()> f);

     /**
      * Creates an empty stream.
      */
     static gen::Stream<T> empty();

     /**
      * The beginning of the Stream's virtual range.
      */
     gen::Iterator<T> begin() const {
         return _begin;
     }

     /**
      * The end of the Stream's virtual range.
      */
     gen::Iterator<T> end() const {
         return gen::end<T>();
     }

     /**
      * Concatenate two streams together into one.
      */
     gen::Stream<T> operator+(const Stream<T>& other) const {
         gen::Iterator<T> iter = gen::end<T>();
         int mode = 0;

         return gen::Stream<T>([=, this]() mutable -> std::optional<T> {
             while (iter == gen::end<T>() && mode < 2) {
                 if (mode == 1) {
                     iter = other.begin();
                 } else {
                     iter = begin();
                 }
                 mode++;
             }

             if (iter == gen::end<T>()) {
                 return {};
             }

             return *(iter ++);
         });
     }

     gen::Stream<T>& operator+=(const Stream<T>& other) {
         auto combined = *(this) + other;
         _begin = combined._begin;
         return *this;
     }

     /**
      * Advance the stream forward n items, effectively skipping n items.
      */
     gen::Stream<T> advance(int n) {
         return gen::Stream<T>(begin().advance(n));
     }

     /**
      * Trim `n` items off of the left of the virtual range.
      */
     gen::Stream<T> trim_left(unsigned int n);

     /**
      * Trim `n` items off of the right of the virtual range.
      *
      * NOTE: right trimming requires the use of a buffer the size of the right
      * trim amount, since virtual ranges are of unknown length.
      */
     gen::Stream<T> trim_right(unsigned int n);

     /**
      * Trim `left` items off of the left and `right` items off of the right
      * of the virtual range and stream the rest into a new Stream.
      *
      * NOTE: right trimming requires the use of a buffer the size of the right
      * trim amount, since virtual ranges are of unknown length.
      */
     gen::Stream<T> trim(unsigned int left, unsigned int right);

     /**
      * Buffer the stream `bufsize` items forward.  The resulting stream elements are all
      * references into the Buffer, with each step shifting the buffer forward one element.
      *
      * @param bufsize The size of the buffer.
      * @param squash Determines how to behave at the end of the scalar sequence.
      *      If `true`, items are removed from the beginning of the buffer each step until
      *      it is empty.  The buffer is allowed to be smaller than `bufsize`.
      *      If `false` (the default), the buffered sequence ends when the scalar sequence
      *      ends.  The buffer is not allowed to be smaller than `bufsize`, thus if the
      *      scalar sequence is smaller than `bufsize`, the stream is empty.
      */
     gen::Stream<std::reference_wrapper<Buffer<T>>> buffer(unsigned int bufsize, bool squash = false);

     /**
      * Scans to the last item of the stream and returns it.
      * If the stream is empty, nothing is returned.
      */
     std::optional<T> last() {
         std::optional<T> last_item = {};

         for (auto value : *this) {
             last_item = value;
         }

         return last_item;
     }

     /**
      * Streams only the first `n` elements of this stream.
      */
     gen::Stream<T> limit(int n) {
         int offset = 0;
         auto iter = begin();

         return gen::Stream<T>([=]() mutable -> std::optional<T> {
             if (offset < n && iter != gen::end<T>()) {
                 offset++;
                 return *iter++;
             }
             return {};
         });
     }

     /**
      * Sort all of the elements in ths stream, then stream from the resulting
      * sorted collection.
      */
     gen::Stream<T> sorted() const {
         std::optional<std::deque<T>> lazy_sorted = {};

         return gen::Stream<T>([=, this]() mutable -> std::optional<T> {
             if (! lazy_sorted.has_value()) {
                 lazy_sorted = std::deque<T>();
                 std::copy(begin(), end(), std::back_inserter(*lazy_sorted));
                 std::sort(lazy_sorted->begin(), lazy_sorted->end());
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

     /**
      * Generate all items in the stream until there are none left but ignore them.
      */
     void drain() {
         for (auto iter = begin(); iter != end(); iter++) { }
     }

     /**
      * Streams a new stream of streams resulting from applying the function `f` to each
      * element in this stream, where `f` generates a stream of results for each element.
      */
     template<class R>
     gen::Stream<gen::Stream<R>> transform_split(std::function<gen::Stream<R>(const T&)> f) const {
         Iterator<T> iter = begin();

         return gen::Stream<gen::Stream<R>>([=, this]() mutable -> std::optional<gen::Stream<R>> {
             if (iter == end()) {
                 return {};
             }
             return f(*iter++);
         });
     }

     /**
      * Streams a new stream of streams resulting from applying the function `f` to each
      * element in this stream, where `f` generates a stream of results for each element.
      */
     gen::Stream<gen::Stream<T>> transform_split(std::function<gen::Stream<T>(const T&)> f) const {
         return transform_split<T>(f);
     }

     /**
      * Transform each element in the stream using the given function.
      * Items for which the function returns an empty optional are skipped.
      */
     template<class R = T>
     gen::Stream<R> transform(std::function<std::optional<R>(const T&)> f) const {
         auto iter = begin();

         return gen::Stream<R>([=, this]() mutable -> std::optional<R> {
             while (iter != end()) {
                 auto result = f(*iter);
                 iter++;
                 if (result.has_value()) {
                     return result.value();
                 }
             }
             return {};
         });
     }

     /**
      * Call the given function for each element in the stream, or until
      * the given function returns `false` to indicate the loop should
      * be terminated early.
      */
     void for_each(std::function<bool(const T&)> f) const {
         auto iter = begin();

         for (auto iter = begin(); iter != end(); iter++) {
             if (! f(*iter)) {
                 break;
             }
         }
     }

     /**
      * Call the given function for each element in the stream.
      */
     void for_each(std::function<void(const T&)> f) const {
         for (auto iter = begin(); iter != end(); iter++) {
             f(*iter);
         }
     }

     /**
      * Transform each element in the stream using the given function.
      */
     template<class R = T>
     gen::Stream<R> transform(std::function<R(const T&)> f) const {
         return transform<R>([=](const T& value) -> std::optional<T> {
             return f(value);
         });
     }

     /**
      * Create a new stream containing only items in this stream which match
      * the predicate.
      */
     gen::Stream<T> filter(std::function<bool(const T&)> predicate) const {
         return transform<T>([=](const T& value) -> std::optional<T> {
             if (predicate(value)) {
                 return value;
             }
             return {};
         });
     }

     /**
      * Returns distinct items only once in the stream.
      */
     gen::Stream<T> unique() const {
         std::set<T> sieve = {};
         Iterator<T> iter = begin();

         return gen::Stream<T>([=, this]() mutable -> std::optional<T> {
             while (iter != end() && sieve.find(*iter) != sieve.end()) {
                 iter++;
             }

             if (iter == end()) {
                 return {};
             }

             sieve.insert(*iter);
             return *iter++;
         });
     }

     /**
      * Collects all of the items in the stream into a new collection (std::vector by default).
      */
     template<template<class, class> class C = std::vector, template<class> class A = std::allocator>
     C<T, A<T>> collect() const {
         C<T, A<T>> result;
         result.insert(result.begin(), begin(), end());
         return result;
     }

     /**
      * Collects all of the pairs from a stream into a mapped collection.
      */
     template<class M>
     M map_collect(std::function<std::pair<typename M::key_type, typename M::mapped_type>(const T& value)> f) const {
         M map;
         for (auto iter = begin(); iter != end(); iter++) {
             map.insert(f(*iter));
         }
         return map;
     }

     template<class K, class V>
     std::unordered_map<K, V> map_collect(std::function<std::pair<K, V>(const T&)> f) const {
         return map_collect<std::unordered_map<K, V>>(f);
     }

     /**
      * Collects all of the items in the stream into an std::vector.
      */
     std::vector<T> collect() const {
         return collect<std::vector>();
     }

     /**
      * Determine if all of the items in the sequence match a predicate.
      */
     bool all(std::function<bool(const T&)> f) const {
         for (auto iter = begin(); iter != end(); iter++) {
             if (! f(*iter)) {
                 return false;
             }
         }

         return true;
     }

     /**
      * Determine if any of the items in the sequence match a predicate.
      */
     bool any(std::function<bool(const T&)> f) const {
         for (auto iter = begin(); iter != end(); iter++) {
             if (f(*iter)) {
                 return true;
             }
         }

         return false;
     }

     /**
      * Determine if none of the items in the sequence match a predicate.
      */
     bool none(std::function<bool(const T&)> f) const {
         for (auto iter = begin(); iter != end(); iter++) {
             if (f(*iter)) {
                 return false;
             }
         }

         return true;
     }

     /**
      * Reduce the items in the sequence to a single value.
      */
     template<class R = T>
     R reduce(std::function<R(const R& acc, const T& value)> f, const R& init = R()) {
         R acc = init;
         for (auto iter = begin(); iter != end(); iter++) {
             acc = f(acc, *iter);
         }
         return acc;
     }

     /**
      * Produce the sum of all of the items in the sequence.
      */
     template<class R = T>
     R sum() {
         return reduce<R>([](const R& acc, const T& value) {
             return acc + value;
         });
     }

     /**
      * Joins all of the items in the stream, as strings, with the given separator.
      */
     std::string join(const std::string& sep) {
         std::ostringstream sb;
         for (auto iter = begin(); iter != end(); iter++) {
             sb << *iter;
             if (std::next(iter) != end()) {
                 sb << sep;
             }
         }
         return sb.str();
     }

     /**
      * Determine if the stream is currently empty.
      */
     bool is_empty() const {
         return _begin() == gen::end<T>();
     }

 protected:
     void init(const gen::Iterator<T>& begin) {
         _begin = begin;
     }

 private:
     gen::Iterator<T> _begin;
};

template<class T>
class InitListStream : public Stream<T> {
 public:
     InitListStream(const std::initializer_list<T>& init_list)
     : Stream<T>(gen::end<T>()), _vec(init_list) {
         this->init(gen::iterate(_vec.begin(), _vec.end()));
     }

 private:
     std::vector<T> _vec;
};

/**
 * Streams values from a mutable closure (i.e. generator).
 * Equivalent to `gen::begin`, but wraps the result in a stream.
 */
template<class T>
gen::Stream<T> stream(Generator<T> lambda) {
    return gen::Stream<T>(gen::begin(lambda));
}

/**
 * Stream the given iterable collection.
 */
template<class C>
gen::Stream<typename C::value_type> stream(const C& coll) {
    return gen::stream<typename C::value_type>(gen::iterate(coll.begin(), coll.end()));
}

template<class T>
gen::InitListStream<T> stream(std::initializer_list<T> init_list) {
    return InitListStream<T>(init_list);
}

template<class T>
gen::Stream<T> one(const T& t) {
    return gen::Stream<T>::singleton(t);
}

template<class T>
gen::Stream<T> nothing() {
    return gen::Stream<T>();
}

template<class T>
inline gen::Stream<T> gen::Stream<T>::lazy_singleton(std::function<T()> f) {
    bool yielded = false;
    return gen::Stream<T>([=]() mutable -> std::optional<T> {
        if (yielded == false) {
            yielded = true;
            return f();
        }
        return {};
    });
}

/**
 * Yield a stream containing one value.
 */
template<class T>
inline gen::Stream<T> gen::Stream<T>::singleton(const T& value) {
    bool yielded = false;
    return gen::Stream<T>([=]() mutable -> std::optional<T> {
        if (yielded == false) {
            yielded = true;
            return value;
        }
        return {};
    });
}

/**
 * Lazily evaluate a stream.
 */
template<class T>
inline gen::Stream<T> gen::Stream<T>::lazy(const typename gen::Stream<T>::Factory& f) {
    return (Stream<Stream<T>>::empty() + Stream<Stream<T>>::lazy_singleton(f)).sum();
}

/**
 * Yield an empty stream.
 */
template<class T>
inline gen::Stream<T> gen::Stream<T>::empty() {
    return gen::Stream(end<T>());
}

/**
 * Create a buffered stream that scrolls a buffered deque view of
 * items through the sequence with the given look-ahead.
 *
 * If squash is True, reaching the end of the sequence will pop items
 * off of the end of the deque until it is empty.  Otherwise,
 * the buffered sequence will end after the end of the main
 * sequence is reached.
 */
template<class T>
inline gen::Stream<std::reference_wrapper<Buffer<T>>> gen::Stream<T>::buffer(unsigned int bufsize, bool squash) {
    Buffer<T> buffer;
    auto iter = begin();
    bool initialized = false;

    return gen::stream<std::reference_wrapper<Buffer<T>>>(
        [=]() mutable -> std::optional<std::reference_wrapper<Buffer<T>>> {
        if (! initialized) {
            while (buffer.size() < bufsize && iter != gen::end<T>()) {
                buffer.push_back(*iter);
                iter++;
            }
            initialized = true;
            // If squash is false and we couldn't fill the buffer, return nothing.
            if (squash || buffer.size() == bufsize) {
                return buffer;
            }

            return {};
        }

        if (iter != gen::end<T>()) {
            buffer.pop_front();
            buffer.push_back(*iter);
            iter++;
            return buffer;

        } else if (squash) {
            buffer.pop_front();
            if (! buffer.empty()) {
                return buffer;
            }
            return {};
        }

        return {};
    });
}

/**
 * Trim the given number of elements off of the left of the stream.
 */
template<class T>
gen::Stream<T> gen::Stream<T>::trim_left(unsigned int n) {
    return advance(n);
}

/**
 * Trim the given number of elements off of the right of the sequence.
 */
template<class T>
gen::Stream<T> gen::Stream<T>::trim_right(unsigned int n) {
    if (n == 0) {
        return gen::Stream<T>(*this);
    }
    return buffer(n+1).template transform<T>([](auto buf) -> T {
        return buf.get().front();
    });
}

/**
 * Trim the given number of elements from the beginning
 * and/or end of the stream.
 */
template<class T>
gen::Stream<T> gen::Stream<T>::trim(unsigned int left, unsigned int right) {
    return trim_left(left).trim_right(right);
}

}  // namespace gen
}  // namespace moonlight

#endif /* !__MOONLIGHT_GENERATOR_H */
