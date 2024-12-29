/*
 * ## log.h: A metric-oriented logging framework with JSON context. -
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday December 17, 2024
 *
 * ## Usage ---------------------------------------------------------
 * `log.h` offers a simple metric-oriented logging framework.  Use it to emit
 * logs about events that occur in a running system.
 *
 * Most logging frameworks are message-based, providing a way to emit logs as
 * descriptive sentences that describe what is happening in a system.  This
 * logger uses a different approach which lends itself toward more direct
 * parsing and querying of logs as metric events.  Human language messages
 * associated with metric events may be attached to the JSON context which is
 * included with each log event.
 *
 * To start using the logger, get a reference to the root logger, then set it
 * up to stream to a log sync.  Any child loggers created from this logger will
 * also sync to this log sync.
 *
 * ```
 * auto log = log::Logger::root();
 * log.sync_to(new log::StreamSync(std::cout));
 *
 * log("MyLogEvent")
 *     .qty(3)
 *     .with("version", 1)
 *     .with("result", JSON().with("code", 200).with("message", "OK"))
 *     .ok();
 * ```
 *
 * This example will print a single line to `std::cout` which will look
 * something like this:
 *
 * ```
 * 2024-12-25T17:20:21Z MyLogEvent 3 {"version": 1, "result": {"code": 200, "message": "OK"}}
 * ```
 *
 * The components of the above log line can be broken down as follows:
 *
 * - `2024-12-25T17:20:21Z`: This is the time when the log was initially
 *   captured in UTC, emitted in ISO 8601 format.  If you wish to capture log
 *   times in a different time zone, pass a different time zone to
 *   `log.zone(z)`.
 * - `MyLogEvent`: This is the name of the log event.
 * - `3`: This is the quantity of the log event, which is `1` by default, but
 *   can be specified via `qty()` or incremented with `operator++` if the
 *   `ok()` call is deferred.
 * - `{ ... }`: This is the JSON context body for the log event.  This can
 *   contain any additional context which is helpful in understanding the log
 *   event.  For example, error events could contain `message` and `stack`
 *   components to provide context about where in the code an error occurred.
 *
 * The root logger may optionally be given a name, which will be emitted as the
 * first component of the log name.  Each log name component is separated by a
 * slash "/" and should ideally not contain spaces for ease of parsing.
 *
 * If you wish to emit logs in a different format or to a source other than an
 * `std::ostream`, replace `log::StreamSync` with your own implementation of
 * `log::LogSync` which implements your desired behavior.
 */

#ifndef __MOONLIGHT_LOG_H
#define __MOONLIGHT_LOG_H

#include "moonlight/date.h"
#include "moonlight/json.h"
#include <sstream>
#include <string>
#include <vector>

namespace moonlight {
namespace log {

using Datetime = moonlight::date::Datetime;
using JSON = moonlight::json::JSON;

// ------------------------------------------------------------------
class LogSync;
class Logger;

// ------------------------------------------------------------------
const int CRITICAL = 50;
const int ERROR = 40;
const int WARNING = 30;
const int OK = 20;
const int DEBUG = 10;
const int NOTSET = 10;

// ------------------------------------------------------------------
class Log {
public:
    Log(const std::string& name, const std::optional<int>& qty = {}, const date::Zone& zone = date::Zone::utc())
    : _name(name), _dt(Datetime::now(zone)), _level(OK), _qty(qty.value_or(1)) { }

    static Log from_json(const JSON& json) {
        Log log(json.get<std::string>("name"));
        log._level = json.get<int>("level");
        log._dt = Datetime::from_isoformat(json.get<std::string>("dt"));
        log._context = json.get<JSON>("context");
        return log;
    }

    const JSON& context() const {
        return _context;
    }

    const Datetime& dt() const {
        return _dt;
    }

    void emit(const std::optional<int>& level = {},
              Logger* alt_target = nullptr); // defined inline below

    void error(Logger* alt_target = nullptr); // defined inline below

    int level() const {
        return _level;
    }

    Log& level(int value) {
        _level = value;
        return *this;
    }

    const std::string& name() const {
        return _name;
    }

    void ok(Logger* alt_target = nullptr); // defined inline below

    int qty() const {
        return _qty;
    }

    Log& qty(int value) {
        _qty = value;
        return *this;
    }

    void target(Logger* logger) {
        _target = logger;
    }

    JSON to_json() const {
        return JSON()
        .set("name", name())
        .set("dt", dt().isoformat())
        .set("level", level())
        .set("context", context());
    }

    template<class T>
    Log& with(const std::string& key, const T& value) {
        _context.set<T>(key, value);
        return *this;
    }

    Log operator++(int) const {
        Log log = *this;
        log._qty ++;
        return log;
    }

    Log& operator++() {
        _qty++;
        return *this;
    }

    Log operator--(int) const {
        Log log = *this;
        log._qty --;
        return log;
    }

    Log& operator--() {
        _qty --;
        return *this;
    }

private:
    std::string _name;
    Datetime _dt;
    int _level;
    int _qty;
    JSON _context;
    Logger* _target;
};


// ------------------------------------------------------------------
class Logger {
public:
    Logger(Logger* parent, const std::string& name, const date::Zone& zone = date::Zone::utc())
    : _parent(parent), _name(name), _zone(zone) { }

    ~Logger(); // defined inline below

    static Logger& fallback() {
        static Logger logger = Logger(nullptr, "");
        return logger;
    }

    static Logger root(const std::string& name = "") {
        return Logger(nullptr, name);
    }

    Logger logger(const std::string& name = "Logger") {
        return Logger(this, name);
    }

    Logger& emit(const Log& log) {
        _sync(log);
        return *this;
    }

    std::string fullname() const {
        std::ostringstream sb;

        if (parent() != nullptr && parent()->name() != "") {
            sb << parent()->fullname() << "/";
        }

        sb << name();
        return sb.str();
    }

    const std::string& name() const {
        return _name;
    }

    Logger* parent() const {
        return _parent;
    }

    const date::Zone& zone() const {
        return _zone;
    }

    Logger& zone(const date::Zone& zone) {
        _zone = zone;
        return *this;
    }

    Logger& sync_to(LogSync* sync) {
        _syncs.push_back(sync);
        return *this;
    }

    Log operator()(const std::string& name, const std::optional<int>& qty = {}) {
        auto log = Log(name, qty, _zone);
        log.target(this);
        return log;
    }

private:
    static Logger setup_fallback_logger(); // defined inline below
    void _sync(const Log& log); // defined inline below

    Logger* _parent = nullptr;
    const std::string _name;
    std::vector<LogSync*> _syncs;
    date::Zone _zone;
};

// ------------------------------------------------------------------
class LogSync {
 public:
     virtual ~LogSync() { }
     virtual void sync(const Logger& logger, const Log& log) = 0;
};

// ------------------------------------------------------------------
class StreamSync : public LogSync {
 public:
     StreamSync(std::ostream& out = std::cout)
     : _out(out), _context_format({.pretty=false, .spacing=true}) { }

     virtual void sync(const Logger& logger, const Log& log) {
         _out << log.dt().isoformat() << " ";
         if (logger.fullname() != "") {
             _out << logger.fullname() << "/";
         }
         _out << log.name()
              << " " << log.qty()
              << " " << json::to_string(log.context(), context_format())
              << std::endl;
     }

     const json::FormatOptions& context_format() const {
         return _context_format;
     }

     void context_format(const json::FormatOptions& options) {
         _context_format = options;
     }

 private:
     std::ostream& _out;
     json::FormatOptions _context_format;
};

/* ---------------------------------------------------------------------------
 * Inline method definitions
 * ------------------------------------------------------------------------- */
inline Logger::~Logger() {
    while (! _syncs.empty()) {
        delete _syncs.back();
        _syncs.pop_back();
    }
}

inline void Logger::_sync(const Log& log) {
    for (auto* sync : _syncs) {
        sync->sync(*this, log);

    }
    if (parent() != nullptr) {
        for (auto* sync : parent()->_syncs) {
            sync->sync(*this, log);
        }
    }
}

inline Logger Logger::setup_fallback_logger() {
    auto logger = Logger(nullptr, "");
    logger.sync_to(new StreamSync(std::cout));
    return logger;
}

inline void Log::emit(const std::optional<int>& level, Logger* alt_target) {
    if (level.has_value()) {
        this->level(*level);
    }

    Logger* target = alt_target == nullptr ? _target : alt_target;
    target->emit(*this);
}

inline void Log::error(Logger* alt_target) {
    level(ERROR);
    Logger* target = alt_target == nullptr ? _target : alt_target;
    target->emit(*this);
}

inline void Log::ok(Logger* alt_target) {
    level(OK);
    Logger* target = alt_target == nullptr ? _target : alt_target;
    target->emit(*this);
}

}
}

#endif /* !__MOONLIGHT_LOG_H */
