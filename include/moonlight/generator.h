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

template<class T>
using Generator = std::function<std::optional<T>()>;

/**
 * A template class allowing the results of a Generator lambda to be
 * wrapped in a standard library compatible iterator type and used with
 * constructs like algorithm templates and the ranged-for loop.
 *
 * Used as a building block for higher level composition abstractions,
 * such as gen::Stream.
 */
template<class T>
class Iterator : public std::iterator<std::input_iterator_tag, T> {
public:
    typedef Generator<T> Closure;

    Iterator() : _value({}), _position(-1) { }
    Iterator(const Closure& closure) : _closure(closure), _value(_closure()) { }
    Iterator(const Iterator& iter)
    : _value(iter._value), _position(iter._position), _closure(iter._closure) { }

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
        for (int x = 0; x < distance && iter != /*end<T>()*/Iterator<T>(); x++, iter++);
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

    int position() const {
        return _position;
    }

    const T& operator*() {
        if (_value.has_value()) {
            return *_value;
        } else {
            THROW(core::UsageError, "Attempted to deference past the end of the sequence.");
        }
    }

    const T* operator->() {
        return &(operator*());
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

/**
 * A queue template for asynchronously communiating the output
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
    auto thread = std::async(std::launch::async, [&] {
        for (auto iter = begin(factory, std::forward(params)...);
             iter != end<T>();
             iter++) {
            queue->yield(*iter);
        }
        queue->complete();
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
    Stream(const gen::Iterator<T> begin) : _begin(begin) { }
    Stream(const typename Iterator<T>::Closure& closure) : _begin(closure) { }

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
     * Merge the output of two streams into a single output stream.
     */
    gen::Stream<T> merge(gen::Stream<T> streamA, gen::Stream<T> streamB);

    /**
     * Merge the output of one or more streams in a stream of streams into
     * a single output stream.
     */
    gen::Stream<T> merge(gen::Stream<gen::Stream<T>> stream_of_streams);

    /**
     * Merge the output from one or more streams into a single output stream.
     */
    template<class... TD>
    static gen::Stream<T> merge(gen::Stream<T> stream, TD... streams);

    /**
     * Merge the output of one or more streams in an iterable collection
     * of streams into a single output stream.
     */
    template<class C>
    static gen::Stream<T> merge(const C& coll);

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
     * Advance the stream forward n items, effectively skipping n items.
     */
    gen::Stream<T> advance(int n) {
        return gen::Stream<T>(begin().advance(n));
    }

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
    gen::Stream<std::reference_wrapper<Buffer<T>>> buffer(int bufsize, bool squash = false);

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

        return gen::Stream<T>([lazy_sorted, this]() mutable -> std::optional<T> {
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
     * Streams a new stream resulting from applying the function `f` to each
     * element in this stream.
     */
    template<class R>
    gen::Stream<gen::Stream<R>> transform_split(std::function<gen::Stream<R>(const T&)> f) const {
        Iterator<T> iter = begin();

        return gen::Stream<gen::Stream<R>>([iter, f, this]() mutable -> std::optional<gen::Stream<R>> {
            if (iter == end()) {
                return {};
            }
            return f(*iter++);
        });
    }

    /**
     * Streams a new stream resulting from applying the function `f` to each
     * element in this stream.
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
        std::optional<R> result;

        return gen::Stream<R>([iter, f, result, this]() mutable -> std::optional<R> {
            while (iter != end()) {
                result = f(*iter);
                iter++;
                if (result.has_value()) {
                    return result.value();
                }
            }
            return {};
        });
    }

    void for_each(std::function<bool (const T&)> f) const {
        auto iter = begin();

        for (auto iter = begin(); iter != end(); iter++) {
            if (! f(*iter)) {
                break;
            }
        }
    }

    void for_each(std::function<void (const T&)> f) const {
        for (auto iter = begin(); iter != end(); iter++) {
            f(*iter);
        }
    }

    /**
     * Transform each element in the stream using the given function.
     */
    template<class R = T>
    gen::Stream<R> transform(std::function<R(const T&)> f) const {
        return transform<R>([f](const T& value) -> std::optional<T> {
            return f(value);
        });
    }

    /**
     * Create a new stream containing only items in this stream which match
     * the predicate.
     */
    gen::Stream<T> filter(std::function<bool(const T&)> predicate) const {
        return transform<T>([predicate](const T& value) -> std::optional<T> {
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

        return gen::Stream<T>([sieve, iter, this]() mutable -> std::optional<T> {
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
     * Collects all of the items in the stream into an std::vector.
     */
    std::vector<T> collect() const {
        return collect<std::vector>();
    }

    /**
     * Joins all of the items in the stream, as strings, with the given separator.
     */
    std::string join(const std::string& sep) {
        return str::join(collect(), sep);
    }

    /**
     * Determine if the stream is currently empty.
     */
    bool is_empty() const {
        return _begin() == gen::end<T>();
    }

private:
    gen::Iterator<T> _begin;
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
inline gen::Stream<T> gen::Stream<T>::lazy_singleton(std::function<T()> f) {
    bool yielded = false;
    return gen::Stream<T>([yielded, f]() mutable -> std::optional<T> {
        if (yielded == false) {
            yielded = true;
            return f();
        }
        return {};
    });
}

template<class T>
gen::Stream<T> gen::Stream<T>::merge(gen::Stream<gen::Stream<T>> stream_of_streams) {
    gen::Iterator<gen::Stream<T>> stream_iter = stream_of_streams.begin();
    gen::Iterator<T> iter = stream_iter == stream_of_streams.end() ? end<T>() : stream_iter->begin();

    return gen::Stream<T>([stream_of_streams, stream_iter, iter]() mutable -> std::optional<T> {
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

template<class T>
gen::Stream<T> gen::Stream<T>::merge(gen::Stream<T> streamA, gen::Stream<T> streamB) {
    auto iterA = streamA.begin();
    auto iterB = streamB.begin();

    return gen::begin([iterA, iterB, streamA, streamB]() mutable -> std::optional<T> {
        if (iterA == streamA.end()) {
            if (iterB == streamB.end()) {
                return {};
            }

            return *(iterB++);
        }

        return *(iterA++);
    });
}

template<class T, class... TD>
gen::Stream<T> merge(gen::Stream<T> streamA, gen::Stream<T> streamB, TD... streams) {
    return merge(streamA, merge(streamB, streams...));
}

template<class T>
inline gen::Stream<T> gen::Stream<T>::lazy(const typename gen::Stream<T>::Factory& f) {
    return gen::merge(lazy_singleton(f));
}

template<class T>
inline gen::Stream<T> gen::Stream<T>::singleton(const T& value) {
    bool yielded = false;
    return gen::Stream<T>([yielded, value]() mutable -> std::optional<T> {
        if (yielded == false) {
            yielded = true;
            return value;
        }
        return {};
    });
}

template<class T>
inline gen::Stream<T> gen::Stream<T>::empty() {
    return gen::Stream(end<T>());
}

template<class T>
template<class... TD>
inline gen::Stream<T> gen::Stream<T>::merge(gen::Stream<T> stream, TD... streams) {
    return gen::merge<T>(stream, streams...);
}

template<class T>
template<class C>
inline gen::Stream<T> gen::Stream<T>::merge(const C& coll) {
    return merge<T>(gen::stream<gen::Stream<T>>(coll));
}

template<class T>
inline gen::Stream<std::reference_wrapper<Buffer<T>>> gen::Stream<T>::buffer(int bufsize, bool squash) {
    Buffer<T> buffer;
    auto iter = begin();
    bool initialized = false;

    return gen::stream<std::reference_wrapper<Buffer<T>>>([=]() mutable -> std::optional<std::reference_wrapper<Buffer<T>>> {
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

template<class T>
gen::Stream<T> gen::Stream<T>::trim(unsigned int left, unsigned int right) {
    gen::Stream<T> stream = *this;

    if (right != 0) {
        stream = stream.buffer(right + 1).template transform<T>([](auto& buf) {
            return buf.get().front();
        });
    }

    if (left != 0) {
        stream = stream.advance(left);
    }

    return stream;
}

}
}

#endif /* !__MOONLIGHT_GENERATOR_H */
