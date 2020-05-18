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
### Third Party Header-only Libraries
These libraries were _not_ written by me, and are provided by their authors
under their respective licenses.

#### [PicoJSON](https://github.com/kazuho/picojson)
- Author: [Kazuho Oku](https://github.com/kazuho)
- License: [2-Clause BSD License](https://github.com/kazuho/picojson/blob/master/LICENSE)

A header-only JSON library.  I use this in conjunction with `moonlight/json.h`
as a handy serialization tool.

#### [Sole](https://github.com/r-lyeh-archived/sole)
- Author: [r-lyeh](https://github.com/r-lyeh/)
- License: [zlib/libpng License](https://github.com/r-lyeh-archived/sole/blob/master/LICENSE)

An elegantly simple UUID v1+v4 generator/parser.

#### [tinyformat](https://github.com/c42f/tinyformat)
- Author: [Chris Foster](https://github.com/c42f)
- License: [Boost Software License 1.0](https://www.boost.org/LICENSE_1_0.txt)
    - As specified [here](https://github.com/c42f/tinyformat#license).

A type-safe printf replacement for C++.  I use this all of the time, and its
an excellent option for folks who like working with C-style format strings.

#### [Date](https://github.com/HowardHinnant/date)
- Author: [Howard Hinnant](https://github.com/HowardHinnant)
- License: [MIT License](https://github.com/HowardHinnant/date/blob/master/LICENSE.txt)

Date and calendar libraries built atop the relatively new `std::chrono`.

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

#### `core.h`
The original toolbox header.  Contains lots of small macros and functions for a
wide variety of common needs.

- `variadic`: Tools for interacting with template parameter packs.
- `str`: String manipulation functions.
- `core`: Core utilities, such as...
    - `Finalizer`: An RAII context wrapper providing "finally" like behavior
      when leaving a C++ scope without having to wrap everything in new objects.
    - Functions for generating and formatting stack traces.
    - `Exception`, a richer base class for runtime exceptions, derived from
      `std::runtime_error`.
    - A few common exception types: `IndexException`, `NotImplementedException`,
      and `AssertionFailure`.
- `splice`: Python-like index splicing for C++ linear containers like `vector`
and `list`.
- `maps`: Utility functions for `map` and `unordered_map`.
- `mmaps`: Utility functions for multimaps.
- `collect`: Functional utilities for linear collections.
- `file`: Wrapper functions for easily reading and writing files, and
  `FileException` for when that doesn't work out.  Also provides `Writable`,
  which lets an object be writable to a stream by implementing `repr()` instead
  of having to repeat the `friend ostream& operator<<` dance.

#### `curses.h`
A set of ncurses wrappers for initialzation and usage.  Greatly reduces the
cognitive load for creating simple ncurses applications with windows and panels.
Also addresses my biggest gripe by replacing Y,X indexing with X,Y indexing for
functions referring to locations on the screen.

#### `debug.h`
Some quirky debugging tools, enabled only if `MOONLIGHT_DEBUG` is defined.

- `debugger` like in Javascript.  Raises a segfault, useful for defining
  breakpoints in code.
- `dbgprint` prints to stderr.

#### `json.h`
Defines `json::Wrapper`, a wrapper around the beautiful
[PicoJSON](https://github.com/kazuho/picojson) library by Kazuho Oku.  Makes
usage just a little bit easier when heterogenous lists aren't required.  This is
super useful for parsing most sanely defined API results and for using JSON as a
settings and object serialization medium.  See `json.cpp` in the test suite for
sample usage.

#### `linked_map.h`
Provides a `linked_map` which behaves just like `unordered_map` except that
insertion order is preserved for iteration.

#### `posix.h`
POSIX specializations for `Timer`.

#### `sdl2.h`
SDL2 specializations for `Timer`.

#### `test.h`
Provides the `TestSuite` unit test harness where tests are defined by lambda
functions.  For copious usage examples, see any of the tests for this library.

#### `time.h`
Defines `Timer`, a generic accumulating timer, and `RelativeTimer` based on
the tickes of another `Timer`.  I use this timer to keep framerate and pace in
games and other simulations.
