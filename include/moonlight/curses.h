/*
 * moonlight/curses.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Saturday February 22, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_CURSES_H
#define __MOONLIGHT_CURSES_H

#include "moonlight/exceptions.h"
#include "moonlight/posix.h"
#include "tinyformat/tinyformat.h"

#include <climits>
#include <functional>
#include <optional>

#include <curses.h>
#include <panel.h>

namespace moonlight {
namespace curses {

using moonlight::time::posix::get_ticks;

//-------------------------------------------------------------------
class CursesError : public core::Exception {
    using Exception::Exception;
};

//-------------------------------------------------------------------
const int MAX_KEYCODE = 410;

//-------------------------------------------------------------------
struct XY {
    int x;
    int y;
};

//-------------------------------------------------------------------
inline XY screen_size() {
    int x, y;
    getmaxyx(stdscr, y, x);
    return {x, y};
}

//-------------------------------------------------------------------
inline XY center_on_area(const XY& sz_A, const XY& sz_B) {
    if (sz_A.x > sz_B.x || sz_A.y > sz_B.y) {
        throw CursesError("Can't center a smaller size inside another.");
    }

    return {
        (sz_B.x - sz_A.x) / 2,
        (sz_B.y - sz_A.y) / 2
    };
}

//-------------------------------------------------------------------
inline XY center_on_screen(const XY& sz) {
    return center_on_area(sz, screen_size());
}

//-------------------------------------------------------------------
inline void cleanup() {
    endwin();
}

//-------------------------------------------------------------------
inline void init() {
    initscr();
    atexit(cleanup);
    cbreak();
    noecho();
    nodelay(stdscr, true);
    keypad(stdscr, true);
    start_color();
}

//-------------------------------------------------------------------
inline void init_color() {
    if (has_colors() == FALSE) {
        throw CursesError("This terminal does not support color.");
    }
    start_color();
    use_default_colors();
}

//-------------------------------------------------------------------
inline void make_color_pairs() {
    init_pair(0, COLOR_BLACK, -1);
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_BLUE, -1);
    init_pair(5, COLOR_MAGENTA, -1);
    init_pair(6, COLOR_CYAN, -1);
    init_pair(7, COLOR_WHITE, -1);

    init_pair(10, COLOR_WHITE, COLOR_WHITE);
    init_pair(11, COLOR_RED, COLOR_WHITE);
    init_pair(12, COLOR_GREEN, COLOR_WHITE);
    init_pair(13, COLOR_YELLOW, COLOR_WHITE);
    init_pair(14, COLOR_BLUE, COLOR_WHITE);
    init_pair(15, COLOR_MAGENTA, COLOR_WHITE);
    init_pair(16, COLOR_CYAN, COLOR_WHITE);
    init_pair(17, COLOR_BLACK, COLOR_WHITE);

    init_pair(20, COLOR_BLACK, COLOR_BLACK);
    init_pair(21, COLOR_BLACK, COLOR_RED);
    init_pair(22, COLOR_BLACK, COLOR_GREEN);
    init_pair(23, COLOR_BLACK, COLOR_YELLOW);
    init_pair(24, COLOR_BLACK, COLOR_BLUE);
    init_pair(25, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(26, COLOR_BLACK, COLOR_CYAN);
    init_pair(27, COLOR_BLACK, COLOR_WHITE);

    init_pair(30, COLOR_WHITE, COLOR_WHITE);
    init_pair(31, COLOR_WHITE, COLOR_RED);
    init_pair(32, COLOR_WHITE, COLOR_GREEN);
    init_pair(33, COLOR_WHITE, COLOR_YELLOW);
    init_pair(34, COLOR_WHITE, COLOR_BLUE);
    init_pair(35, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(36, COLOR_WHITE, COLOR_CYAN);
    init_pair(37, COLOR_WHITE, COLOR_BLACK);
}

//-------------------------------------------------------------------
inline void init_mouse() {
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    std::cout << "\033[?1003h\n" << std::endl;
}

//-------------------------------------------------------------------
inline int get_keycode(const std::string& named_key) {
    for (int x = 0; x <= MAX_KEYCODE; x++) {
        auto name = keyname(x);
        if (name != NULL && name == named_key) {
            return x;
        }
    }

    return ERR;
}

//-------------------------------------------------------------------
inline std::string get_keycode_name(int keycode) {
    auto name = keyname(keycode);
    return name == nullptr ? "NULL" : name;
}

//-------------------------------------------------------------------
inline void dbg_print_all_keycodes() {
    for (int x = 0; x <= MAX_KEYCODE; x++) {
        auto name = keyname(x);
        tfm::printf("%d\t%s\n", x, name == NULL ? "NULL" : name);
    }
}

//-------------------------------------------------------------------
inline void show_cursor() {
    curs_set(1);
}

//-------------------------------------------------------------------
inline void hide_cursor() {
    curs_set(0);
}

//-------------------------------------------------------------------
class Window {
public:
    Window(XY sz_) :
    Window(sz_, center_on_screen(sz_)) { }
    Window(XY sz_, XY pos_) {
        _window = newwin(sz_.y, sz_.x, pos_.y, pos_.x);
    }

    Window(const Window& win) {
        _window = dupwin(win._window);
    }

    Window(WINDOW* window) {
        _window = window;
    }

    static Window root() {
        return Window(::stdscr);
    }

    ~Window() {
        if (_window != ::stdscr) {
            delwin(_window);
        }
    }

    XY sz() const {
        XY sz;
        getmaxyx(_window, sz.y, sz.x);
        return sz;
    }

    XY pos() const {
        XY pos;
        getbegyx(_window, pos.y, pos.x);
        return pos;
    }

    WINDOW* raw_ptr() {
        return _window;
    }

    Window& pos(const XY& pos_, bool move=true) {
        if (move) {
            mvwin(raw_ptr(), pos_.y, pos_.x);
        }
        return *this;
    }

    Window& center(const Window& other_window) {
        return pos(center_on_area(sz(), other_window.sz()));
    }

    void erase() {
        werase(raw_ptr());
    }

    XY where() const {
        XY pos;
        getyx(_window, pos.y, pos.x);
        return pos;
    }

    void go(const XY& pos, bool absolute = false) {
        go(pos.x, pos.y, absolute);
    }

    void go(int x, int y, bool absolute = false) {
        if (! absolute) {
            if (x < 0) x += sz().x;
            if (y < 0) y += sz().y;
        }
        wmove(_window, y, x);
    }

    void clear() {
        wclear(raw_ptr());
    }

    void draw(const std::string& chars, int attr = A_NORMAL) {
        wattrset(raw_ptr(), attr);
        for (char c : chars) {
            waddch(raw_ptr(), c);
        }
        wattrset(raw_ptr(), A_NORMAL);
    }

    void carousel(const std::string& chars, bool forward = true, int interval = 250) {
        for (size_t x = 0; x < chars.length(); x++) {
            if (forward) {
                wattrset(raw_ptr(), COLOR_PAIR((get_ticks() / interval - x) % 7 + 1));
            } else {
                wattrset(raw_ptr(), COLOR_PAIR((get_ticks() / interval + x) % 7 + 1));
            }
            waddch(raw_ptr(), chars[x]);
        }
        wattrset(raw_ptr(), A_NORMAL);
    }

    void draw_border(int attr = A_NORMAL) {
        wattrset(raw_ptr(), attr);
        box(raw_ptr(), 0, 0);
        wattrset(raw_ptr(), A_NORMAL);
    }

private:
    WINDOW* _window;
};

//-------------------------------------------------------------------
class Panel : public std::enable_shared_from_this<Panel> {
public:
    Panel(int x, int y) : Panel(XY({x, y})) { }

    Panel(const XY& sz_) : _win(Window(sz_)) {
        init_own_panel();
    }

    Panel(const XY& sz_, const XY& pos_) : _win(Window(sz_, pos_)) {
        init_own_panel();
    }

    Panel(const Window& win_) : _win(win_) {
        init_own_panel();
    }

    ~Panel() {
        del_panel(raw_ptr());
    }

    const Window& cwin() const {
        return _win;
    }

    XY sz() const {
        return cwin().sz();
    }

    XY pos() const {
        return cwin().pos();
    }

    std::shared_ptr<Panel> pos(const XY& pos_) {
        move_panel(raw_ptr(), pos_.y, pos_.x);
        return shared_from_this();
    }

    std::shared_ptr<Panel> pos(int x, int y) {
        return pos({x, y});
    }

    std::shared_ptr<Panel> center(const Window& window) {
        win().center(window);
        return shared_from_this();
    }

    Window& win() {
        return _win;
    }

    void to_top() {
        top_panel(raw_ptr());
    }

    void to_bottom() {
        bottom_panel(raw_ptr());
    }

    PANEL* raw_ptr() {
        return _panel;
    }

private:
    void init_own_panel() {
        _panel = new_panel(win().raw_ptr());
    }

    Window _win;
    PANEL* _panel;
};

}
}

#endif /* !__MOONLIGHT_CURSES_H */
