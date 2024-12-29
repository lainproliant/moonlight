/*
 * ## ansi.h - Tools for printing ANSI escaped characters. ----------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday, May 18, 2018
 *
 * Distributed under terms of the MIT license.
 *
 * ## Dependencies --------------------------------------------------
 * To install dependencies, run `./build.py deps` at the moonlight project root.
 *
 * - SRELL (via `color.h`)
 *   - Add `moonlight/deps/SRELL` to your C++ include path.
 *
 * ## Usage: Colors -------------------------------------------------
 * The recommended way to use the methods in `color.h` is to import the
 * `moonlight::fg` and `moonlight::bg` namespaces into your working namespace,
 * like so:
 *
 * ```
 * using fg = moonlight::fg;
 * using bg = moonlight::bg;
 * ```
 *
 * To emit color highlighted text, use the text wrapper methods in `fg::*` and
 * `bg::*`.  These methods will create `ansi::Decorator` function objects which,
 * when provided with text, will emit `ansi::WrappedText` string wrappers which,
 * when emitted via a `std::ostream::operator<<` will print the given text with
 * the appropriate beginning and end ANSI escape codes.
 *
 * The simplest way to emit text in the standard 8 ANSI colors is to use the
 * methods for those colors in `fg::*` and `bg::*`, namely:
 *      - `black`, `color(0)`
 *      - `red`, `color(1)``
 *      - `green`, `color(2)`
 *      - `yellow`, `color(3)``
 *      - `blue`, `color(4)`
 *      - `magenta`, `color(5)`
 *      - `cyan`, `color(6)`
 *      - `white`, `color(7)`
 *
 * Meta functions also exist in `fg::*` for adding effects, namely:
 *      - `fg::bright`
 *      - `fg::dim`
 *      - `fg::underscore`
 *      - `fg::blink`
 *      - `fg::reverse`
 *      - `fg::hidden`
 *
 *  To use these effects, make a new function via composition, e.g.:
 *
 *  ```
 *  const auto bright_red = fg::bright(fg::red);
 *  std::cout << bright_red("Hello, I'm Red!") << std::endl;
 *  ```
 *
 *  You can also use `fg::rgb` and `bg::rgb` to emit 24bit color on terminals
 *  which support this feature.  This function template can be called with three
 *  integers, one for R, G, and B bytes respectively, or with a
 *  `moonlight::color::uRGB` object.
 *
 *  This example uses `bg::rgb` to print a bright 60-char rainbow line in the
 *  terminal starting at red:
 *
 *  ```
 *  for (int h = 0; h < 60; h++) {
        color::fHSV hsv = { (360.0f * (h / 60.0f)), 1.0, 1.0 };
        auto rgb = static_cast<color::uRGB>(hsv);
        std::cout << bg::rgb(rgb)(" ");
 *  }
 *  ```
 *
 * ## Usage: Control ------------------------------------------------
 * Several screen control sequences are provided in `scr::*`.
 *
 * - `clear`: Clears the screen of all text.
 * - `hide_cursor`: Hides the text cursor.
 * - `show_cursor`: Shows the text cursor.
 * - `save_cursor`: Saves the cursor position in the terminal.
 * - `restore_cursor`: Moves the cursor back to its saved position.
 * - `move_cursor(x, y)`: Moves the cursor to the given screen posotion.
 * - `move_cursor_up(n)`: Moves the cursor up a number of lines.
 * - `move_cursor_down(n)`: Moves the cursor down a number of lines.
 * - `move_cursor_left(n)`: Moves the cursor left a number of lines.
 * - `move_cursor_right(n)`: Moves the cursor right a number of lines.
 *
 * The results are `ansi::Sequence` objects that need to be emitted to the
 * TTY output stream to have any effect.  As an example, this is how you could
 * implement the `clear` command (`cls` in DOS):
 *
 * ```
 * std::cout << seq::clear << seq::move_cursor(0, 0);
 * ```
 *
 * ## Options -------------------------------------------------------
 * By default, `ansi.h` respects the following environment variables and
 * conditions for determining when to emit color or control (i.e. text effects,
 * cursor movement, etc.) sequences:
 *
 * - Color codes will be emitted if the output is a TTY and the `NO_COLOR` env
 *   variable is not defined.
 * - Control codes will be emitted if the output is a TTY or the `FORCE_ANSI`
 *   env var is defined.
 *
 * If you wish to override this behavior, fetch the global `ansi::Options`
 * object via `ansi::Options::get()` and call the following methods:
 *
 * - `color_enabled(bool)`: If `true`, color codes will always be emitted,
 *   regardless of env variables, and even if the output stream is not a TTY.
 *   If `false`, color codes will never be emitted.
 * - `control_enabled(bool)`: If `true`, control codes will always be emitted,
 *   regardless of env variables, and even if the output stream is not a TTY.
 *   If `false`, control codes will never be emitted.
 */

#ifndef __MOONLIGHT_ANSI_H
#define __MOONLIGHT_ANSI_H

#include <vector>
#include <string>

#include "moonlight/string.h"
#include "moonlight/tty.h"
#include "moonlight/cli.h"
#include "moonlight/variadic.h"
#include "moonlight/color.h"

namespace moonlight {
namespace ansi {

// ------------------------------------------------------------------
class Options {
public:
    static Options& get() {
        static Options options;
        return options;
    }

    bool color_enabled() {
        if (!_no_color.has_value()) {
            _no_color = cli::getenv("NO_COLOR").has_value();
        }

        return _force_color || (! _no_color.value_or(false));
    }

    Options& color_enabled(bool value) {
        if (value) {
            _no_color = false;
            _force_color = true;
        } else {
            _no_color = true;
            _force_color = false;
        }
        return *this;
    }

    bool control_enabled(std::ostream& out) {
        if (! _force_ansi.has_value()) {
            auto env_val = cli::getenv("FORCE_ANSI");
            _force_ansi = cli::getenv("FORCE_ANSI").has_value();
        }

        return !_suppress_control && (is_tty(out) || _force_ansi.value_or(false));
    }

    Options& control_enabled(bool value) {
        if (value == true) {
            _force_ansi = true;
            _suppress_control = false;

        } else {
            _force_ansi = false;
            _suppress_control = true;
        }

        return *this;
    }

private:
    std::optional<bool> _no_color;
    std::optional<bool> _force_ansi;
    bool _force_color = false;
    bool _suppress_control = false;
};

// ------------------------------------------------------------------
class Sequence {
 public:
     Sequence(const std::string& s, bool control = false)
     : _s(s), _control(control) { }

     Sequence control() const {
         return Sequence(_s, true);
     }

     Sequence operator+(const Sequence& rhs) const {
         return Sequence(_s + rhs._s, _control);
     }

     bool operator==(const Sequence& rhs) const {
         return _s == rhs._s;
     }

     friend std::ostream& operator<<(std::ostream& out, const Sequence& seq) {
         auto& opts = Options::get();
         if ((! seq._control && opts.color_enabled()) || (seq._control && opts.control_enabled(out))) {
             out << seq._s;
         }
         return out;
     }

 private:
     const std::string _s;
     const bool _control;
};

// ------------------------------------------------------------------
template<class T>
std::string as_str(const T& val) {
    std::ostringstream sb;
    sb << val;
    return sb.str();
}

inline Sequence seq() {
    return Sequence("");
}

template<class T>
Sequence seq(const T& val) {
    return Sequence("\x1b[" + as_str(val));
}

template<class T, class... TD>
Sequence seq(const T& val, TD... vals) {
    return seq(val) + seq(vals...);
}

// ------------------------------------------------------------------
template<class T, class... TD>
Sequence attr(const T& val, TD... vals) {
    std::vector<std::string> vec;
    vec.push_back(as_str(val));
    variadic::pass{(vec.push_back(as_str(vals)), 0)...};
    return seq(str::join(vec, ";") + "m");
}

// ------------------------------------------------------------------
const auto clrscr = seq("2J");
const auto clreol = seq("K");
const auto reset = attr(0);
const auto bright = attr(1);
const auto dim = attr(2);
const auto underscore = attr(4);
const auto blink = attr(5);
const auto reverse = attr(7);
const auto hidden = attr(8);

// ------------------------------------------------------------------
inline Sequence rgb(int code, int r, int g, int b) {
    return attr(code, 2, r, g, b);
}

// ------------------------------------------------------------------
inline Sequence rgb(int code, int color) {
    return rgb(code, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

// ------------------------------------------------------------------
inline Sequence rgb(int code, const color::uRGB& color) {
    return rgb(code, color.r, color.g, color.b);
}

// ------------------------------------------------------------------
class WrappedText {
 public:
     WrappedText(const Sequence& start, const std::string& text,
                 const Sequence& end)
     : _start(start), _text(text), _end(end) { }

     friend std::ostream& operator<<(std::ostream& out, const WrappedText& wt) {
         out << wt._start << wt._text << wt._end;
         return out;
     }

     const Sequence& start() const {
         return _start;
     }

     const std::string& text() const {
         return _text;
     }

     const Sequence& end() const {
         return _end;
     }

 private:
     const Sequence _start;
     const std::string _text;
     const Sequence _end;
};

// ------------------------------------------------------------------
class Decorator {
 public:
     Decorator(const Sequence& start, const Sequence& end = reset)
     : _start(start), _end(end) { }

     Decorator operator+(const Decorator& rhs) const {
         return Decorator(
             _start == rhs._start ? _start : _start + rhs._start,
             _end == rhs._end ? _end : rhs._end + _end);
     }

     Decorator operator+(const Sequence& seq) const {
         return Decorator(_start + seq, _end);
     }

     WrappedText operator()(const std::string& text) const {
         return WrappedText(_start, text, _end);
     }

     WrappedText operator()(const WrappedText& text) const {
         return WrappedText(
             _start == text.start() ? _start : _start + text.start(),
             text.text(),
             _end == text.end() ? _end : text.end() + _end);
     }

     Decorator operator()(const Decorator& other) const {
         return *this + other;
     }

 private:
     const Sequence _start;
     const Sequence _end;
};

}  // namespace ansi

namespace fg {

// ------------------------------------------------------------------
inline ansi::Decorator color(int n) {
    return ansi::attr(30 + n);
}

// ------------------------------------------------------------------
template<class... TD>
ansi::Decorator rgb(TD... params) {
    return ansi::rgb(38, params...);
}

const ansi::Decorator bright = ansi::bright;
const ansi::Decorator dim = ansi::dim;
const ansi::Decorator underscore = ansi::underscore;
const ansi::Decorator blink = ansi::blink;
const ansi::Decorator reverse = ansi::reverse;
const ansi::Decorator hidden = ansi::hidden;

const auto black = color(0);
const auto red = color(1);
const auto green = color(2);
const auto yellow = color(3);
const auto blue = color(4);
const auto magenta = color(5);
const auto cyan = color(6);
const auto white = color(7);

}  // namespace fg

namespace bg {

// ------------------------------------------------------------------
inline ansi::Decorator color(int n) {
    return ansi::attr(40 + n);
}

// ------------------------------------------------------------------
template<class... TD>
ansi::Decorator rgb(TD... params) {
    return ansi::rgb(48, params...);
}

const auto black = color(0);
const auto red = color(1);
const auto green = color(2);
const auto yellow = color(3);
const auto blue = color(4);
const auto magenta = color(5);
const auto cyan = color(6);
const auto white = color(7);

}  // namespace bg

namespace scr {

// ------------------------------------------------------------------
const auto clear = ansi::seq("2J");
const auto hide_cursor = ansi::seq("?25l").control();
const auto show_cursor = ansi::seq("?25h").control();
const auto save_cursor = ansi::seq("s");
const auto restore_cursor = ansi::seq("u");

// ------------------------------------------------------------------
inline ansi::Sequence move_cursor(int x, int y) {
    return ansi::seq(y, ";", x, "H");
}

inline ansi::Sequence move_cursor_up(int n) {
    return ansi::seq(n, "A");
}

inline ansi::Sequence move_cursor_down(int n) {
    return ansi::seq(n, "B");
}

inline ansi::Sequence move_cursor_right(int n) {
    return ansi::seq(n, "C");
}

inline ansi::Sequence move_cursor_left(int n) {
    return ansi::seq(n, "D");
}

}  // namespace scr
}  // namespace moonlight

#endif /* __MOONLIGHT_ANSI_H */
