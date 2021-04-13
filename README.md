# Moonlight
Moonlight is a meta library toolbox for C++ projects.  It is intended for use
in my own projects, but made available in case it is useful for anyone else.
It is a mixture of small header-only libraries I have built and submodules
referring mostly to other third-party header only libraries.

## License
Moonlight components written by me (Lain Musgrove) or other first-party
contributors are released under the MIT license, see LICENSE.  Third party
submodules are released under their respective licenses.

## Contents
### Moonlight Header-only Library
These are the headers I've written with useful templates, macros, and tools.

#### `ansi.h`
Tools for printing ANSI escape sequences and colorful CLI text.  See `ansi.cpp`
in the tests for usage examples.

#### `automata.h`
Templates for finite-state automata, a.k.a. state machines, with states modeled
as runnable object states that are tied together under a common context.  I use
this as the basis for experimenting with games, parsers, and other runtime
engines.

#### `cli.h`
Useful tools for building command line apps.  Provides  `CommandLine`, which is
a very simple command line parser supporting short and long options similar to
`getopt_long`.

#### `collect.h`
Various tools for working with linear collections.

#### `color.h`
Methods for RGB<->HSV color conversions.

#### `curses.h`
A set of ncurses wrappers for initialzation and usage.  Greatly reduces the
cognitive load for creating simple ncurses applications with windows and panels.
Also addresses my biggest gripe by replacing Y,X indexing with X,Y indexing for
functions referring to locations on the screen.

#### `date.h`
A datetime library utilizing Howard Hinnant's `date` and providing an interface
more akin to Python's `datetime`.

#### `debug.h`
Some quirky debugging tools, enabled only if `MOONLIGHT_DEBUG` is defined.

- `debugger` like in Javascript.  Raises a segfault, useful for defining
  breakpoints in code.
- `dbgprint` prints to stderr.

#### `exceptions.h`
Defines a smarter base exception class derived from `std::runtime_error`, along
with a few derived exception types for common use.

#### `file.h`
Includes utilities for working with files and byte streams.  Includes
`BufferedInput` which is quite useful for parsers.

#### `generator.h`
Implements a pattern similar to Python's generators.  Wraps lambda-based
sequence generation in a `for-in` compatible standard iterator interface.
Also offers `Queue`, a thread-safe queue, and methods for wrapping generators in
async queues.

#### `hash.h`
Contains `combine` which is copied from `boost::hash_combine`.  May be a home
for other useful hashing functions in the future.

#### `json.h`
*NEW* A standalone JSON parser and object model inspired by PicoJSON.  This was
originally a wrapper around PicoJSON but is now a full JSON parser and
value serializer.

#### `lex.h`
A generic recursive lexical scanner based on my `lexex` Python module.

#### `linked_map.h`
Provides a `linked_map` which behaves just like `unordered_map` except that
insertion order is preserved for iteration.

#### `make.h`
An optional free-function template `make`, shorthand for `std::make_shared`.

#### `maps.h`
Various tools for working with associative containers.

#### `meta.h`
Tools for template metaprogramming and compile-time static assertions for
templates.

#### `mmap.h`
Functions for building and working with multimap containers.

#### `posix.h`
POSIX specializations for `Timer`.

#### `sdl2.h`
SDL2 specializations for `Timer`.

#### `slice.h`
Implements `slice` and `slice_offset` which offer Python-style slicing and item
access for linear containers.

#### `string.h`
String utility functions such as `join`, `split`, `trim`, and others.

#### `system.h`
Provides system utilities such as a version of `getenv` which returns an
`std::optional<std::string>`.

#### `test.h`
Provides the `TestSuite` unit test harness where tests are defined by lambda
functions.  For copious usage examples, see any of the tests for this library.

#### `time.h`
Defines `Timer`, a generic accumulating timer, and `RelativeTimer` based on
the tickes of another `Timer`.  I use this timer to keep framerate and pace in
games and other simulations.

#### `tty.h`
From ("fileno(3) on C++ Streams: A Hacker's Lament")[https://www.ginac.de/~kreckel/fileno/]
by Richard B. Kreckel.  Also includes a free-function template `is_tty` which
can be used to determine if an `std::basic_ios` stream represents a tty.

#### `variadic.h`
Provides utilities for variadic template parameter expansion such as `pass` and
`to_vector`.

### Third Party Libraries
#### [inifile-cpp](https://github.com/Rookfighter/inifile-cpp)
- Author: [Fabian Meyer](https://github.com/Rookfighter)
- License [MIT](https://github.com/Rookfighter/inifile-cpp/LICENSE.txt)

A simple header-only INI file reader and writer.

#### [Date](https://github.com/HowardHinnant/date)
- Author: [Howard Hinnant](https://github.com/HowardHinnant)
- License: [MIT License](https://github.com/HowardHinnant/date/blob/master/LICENSE.txt)

Date and calendar libraries built atop the relatively new `std::chrono`.  Used
as the basis of the `moonlight/date.h` library.

#### [Sole](https://github.com/r-lyeh-archived/sole)
- Author: [r-lyeh](https://github.com/r-lyeh/)
- License: [zlib/libpng License](https://github.com/r-lyeh-archived/sole/blob/master/LICENSE)

An elegantly simple UUID v1+v4 generator/parser.

#### [tinyformat](https://github.com/c42f/tinyformat)
- Author: [Chris Foster](https://github.com/c42f)
- License: [Boost Software License 1.0](https://www.boost.org/LICENSE_1_0.txt)
    - As specified [here](https://github.com/c42f/tinyformat#license).

A type-safe printf replacement for C++.  I use this all of the time, and it's
an excellent option for folks who like working with C-style format strings.

