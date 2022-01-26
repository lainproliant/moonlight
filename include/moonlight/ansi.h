#ifndef __MOONLIGHT_ANSI_H
#define __MOONLIGHT_ANSI_H

#include "moonlight/string.h"
#include "moonlight/exceptions.h"
#include "moonlight/tty.h"
#include "moonlight/cli.h"
#include "moonlight/variadic.h"
#include "moonlight/color.h"
#include <unordered_map>

namespace moonlight {
namespace ansi {

/** -----------------------------------------------------------------
 */
inline bool enabled() {
    static const bool no_color = cli::getenv("NO_COLOR").has_value();
    static const bool force_color = cli::getenv("FORCE_COLOR").has_value();
    return !no_color || force_color;
}

/** -----------------------------------------------------------------
 */
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
        if ((enabled() || seq._control) && is_tty(out)) {
            out << seq._s;
        }
        return out;
    }

private:
    const std::string _s;
    const bool _control;
};

/** -----------------------------------------------------------------
 */
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

/** -----------------------------------------------------------------
 */
template<class T, class... TD>
Sequence attr(const T& val, TD... vals) {
    std::vector<std::string> vec;
    vec.push_back(as_str(val));
    variadic::pass{(vec.push_back(as_str(vals)), 0)...};
    return seq(str::join(vec, ";") + "m");
}

/** -----------------------------------------------------------------
 */
const auto clrscr = seq("2J");
const auto clreol = seq("K");
const auto reset = attr(0);
const auto bright = attr(1);
const auto dim = attr(2);
const auto underscore = attr(4);
const auto blink = attr(5);
const auto reverse = attr(7);
const auto hidden = attr(8);

/** -----------------------------------------------------------------
 */
inline Sequence rgb(int code, int r, int g, int b) {
    return attr(code, 2, r, g, b);
}

/** -----------------------------------------------------------------
 */
inline Sequence rgb(int code, int color) {
    return rgb(code, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
}

/** -----------------------------------------------------------------
 */
inline Sequence rgb(int code, const color::uRGB& color) {
    return rgb(code, color.r, color.g, color.b);
}

/** -----------------------------------------------------------------
 */
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

/** -----------------------------------------------------------------
 */
class Decorator {
public:
    Decorator(const Sequence& start, const Sequence& end = reset)
    : _start(start), _end(end) { }

    Decorator operator+(const Decorator& rhs) const {
        return Decorator(
            _start == rhs._start ? _start : _start + rhs._start,
            _end == rhs._end ? _end : rhs._end + _end
        );
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

}

namespace fg {

/** -----------------------------------------------------------------
 */
inline ansi::Decorator color(int n) {
    return ansi::attr(30 + n);
}

/** -----------------------------------------------------------------
 */
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

}

namespace bg {
/** -----------------------------------------------------------------
 */
inline ansi::Decorator color(int n) {
    return ansi::attr(40 + n);
}

/** -----------------------------------------------------------------
 */
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

}

namespace scr {
const auto hide_cursor = ansi::seq("?25l").control();
const auto show_cursor = ansi::seq("?25h").control();
const auto save_cursor = ansi::seq("s");
const auto restore_cursor = ansi::seq("u");

/** -----------------------------------------------------------------
 */
inline ansi::Sequence move_cursor(int x, int y) {
    return ansi::seq(y, ";", x, "H");
}

/** -----------------------------------------------------------------
 */
inline ansi::Sequence move_cursor_up(int n) {
    return ansi::seq(n, "A");
}

/** -----------------------------------------------------------------
 */
inline ansi::Sequence move_cursor_down(int n) {
    return ansi::seq(n, "B");
}

/** -----------------------------------------------------------------
 */
inline ansi::Sequence move_cursor_right(int n) {
    return ansi::seq(n, "C");
}

/** -----------------------------------------------------------------
 */
inline ansi::Sequence move_cursor_left(int n) {
    return ansi::seq(n, "D");
}

}

}

#endif /* __MOONLIGHT_ANSI_H */
