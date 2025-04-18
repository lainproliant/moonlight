/*
 * ## uuid.h: A wrapper around `stduuid` for generating UUIDs. ------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday September 10, 2023
 *
 * ## Dependencies --------------------------------------------------
 * To install dependencies, run `./build.py deps` at the moonlight project root.
 *
 * - stduuid
 *   - Add `moonlight/deps` to your C++ include path.
 *
 * ## Usage ---------------------------------------------------------
 * This header provides methods for generating and datatypes for representing
 * UUIDs, globally unique identifiers conforming to RFC-9562.  It provides
 * wrappers around the `stduuid` library.  The following types and classes are
 * provided:
 *
 * - `uuid::UUID`: A type alias for `uuids::uuid`.
 * - `uuid::Generator`: A function class with `operator()` for generating UUIDs.
 *   Acts as a container for the random device and seeded generator.
 */

#ifndef __MOONLIGHT_UUID_H
#define __MOONLIGHT_UUID_H

#include "stduuid/include/uuid.h"

namespace moonlight {
namespace uuid {

/**
 * A typedef UUID and an ostream operator for uuids::uuid.
 */
typedef uuids::uuid UUID;

inline std::ostream& operator<<(std::ostream& out, const UUID& uuid) {
    out << uuids::to_string(uuid);
    return out;
}

/**
 * A generator functor encapsulating default crypto-random UUID generation.
 */
class Generator {
 public:
     Generator() : _generator(_init_generator()) {}

     UUID operator()() {
         return _generator();
     }

 private:
     static uuids::uuid_random_generator _init_generator() {
         std::random_device rd;
         auto seed_data = std::array<int, std::mt19937::state_size>{};
         std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
         std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
         std::mt19937 generator(seq);
         return uuids::uuid_random_generator(generator);
     }

     uuids::uuid_random_generator _generator;
};

}  // namespace uuid
}  // namespace moonlight


#endif /* !__MOONLIGHT_UUID_H */
