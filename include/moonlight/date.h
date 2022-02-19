/*
 * date.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday December 10, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_DATE_H
#define __MOONLIGHT_DATE_H

#include "moonlight/generator.h"
#include "moonlight/exceptions.h"
#include "moonlight/maps.h"
#include "moonlight/slice.h"
#include "date/date.h"
#include <map>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <chrono>
#include <regex>

#ifndef MOONLIGHT_TZ_UNSAFE
#include <thread>
#endif

namespace moonlight {
namespace date {

const std::string DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S %Z";
const std::string DATETIME_8601_UTC = "%FT%TZ";
const std::string DATE_FORMAT = "%Y-%m-%d";
typedef std::chrono::duration<long long, std::milli> Millis;

// --------------------------------------------------------
inline std::ostream& operator<<(std::ostream& out, const struct tm& tm_dt) {
    out << "(struct_tm)<"
        << "sec=" << tm_dt.tm_sec << ", "
        << "min=" << tm_dt.tm_min << ", "
        << "hour=" << tm_dt.tm_hour << ", "
        << "mday=" << tm_dt.tm_mday << ", "
        << "mon=" << tm_dt.tm_mon << ", "
        << "year=" << tm_dt.tm_year << ", "
        << "wday=" << tm_dt.tm_wday << ", "
        << "yday=" << tm_dt.tm_yday << ", "
        << "isdst=" << tm_dt.tm_isdst << ", "
        << "gmtoff=" << tm_dt.tm_gmtoff << ", "
        << "zone=" << (tm_dt.tm_zone == nullptr ? "null" : tm_dt.tm_zone) << ">";
    return out;
}

// --------------------------------------------------------
inline bool struct_tm_is_empty(const struct tm& tm_dt) {
    return tm_dt.tm_zone == NULL &&
           tm_dt.tm_year == 0 &&
           tm_dt.tm_mon == 0 &&
           tm_dt.tm_mday == 0 &&
           tm_dt.tm_wday == 0 &&
           tm_dt.tm_yday == 0 &&
           tm_dt.tm_hour == 0 &&
           tm_dt.tm_min == 0 &&
           tm_dt.tm_sec == 0;
}

// --------------------------------------------------------
enum class Month {
    January,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December
};

// --------------------------------------------------------
enum class Weekday {
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday
};

// --------------------------------------------------------
inline int last_day_of_month(int year, Month month) {
    switch(month) {
    case Month::April:
    case Month::June:
    case Month::November:
    case Month::September:
        return 30;

    case Month::January:
    case Month::March:
    case Month::May:
    case Month::July:
    case Month::August:
    case Month::October:
    case Month::December:
        return 31;

    case Month::February:
        return ((!(year % 4) && year % 100) || !(year % 400)) ? 29 : 28;
    }
}

// --------------------------------------------------------
class Zone {
public:
    Zone(const std::string& tz_name) : _tz_name(tz_name) { }

#ifndef MOONLIGHT_TZ_UNSAFE
    static std::mutex& mutex() {
        static std::mutex mutex;
        return mutex;
    }
#endif

    static const Zone& local() {
        static Zone zone = Zone();
        return zone;
    }

    static const Zone& utc() {
        static Zone utc_zone = Zone("UTC");
        return utc_zone;
    }

    std::string name(bool is_dst = false) const {
#ifndef MOONLIGHT_TZ_UNSAFE
        std::lock_guard<std::mutex> lock(mutex());
#endif
        auto env_tz = Zone::get_env_tz();
        set_env_tz(_tz_name);
        std::string sys_tz_name(tzname[is_dst ? 1 : 0]);
        set_env_tz(env_tz);
        return sys_tz_name;
    }

    struct tm mk_struct_tm(Millis ms) const {
        return mk_struct_tm(::date::floor<std::chrono::seconds>(ms).count());
    }

    struct tm mk_struct_tm(time_t utime) const {
#ifndef MOONLIGHT_TZ_UNSAFE
        std::lock_guard<std::mutex> lock(mutex());
#endif
        struct tm local_time;
        auto env_tz = Zone::get_env_tz();
        set_env_tz(_tz_name);
        local_time = *localtime(&utime);
        set_env_tz(env_tz);
        return local_time;
    }

    time_t mk_timestamp(struct tm local_time) const {
#ifndef MOONLIGHT_TZ_UNSAFE
        std::lock_guard<std::mutex> lock(mutex());
#endif
        auto env_tz = get_env_tz();
        set_env_tz(_tz_name);
        time_t utime = mktime(&local_time);
        set_env_tz(env_tz);
        return utime;
    }

    std::string strftime(const std::string& format, struct tm local_time) const {
#ifndef MOONLIGHT_TZ_UNSAFE
        std::lock_guard<std::mutex> lock(mutex());
#endif
        const int bufsize = 256;
        static char buffer[bufsize];
        auto env_tz = get_env_tz();
        set_env_tz(_tz_name);
        ::strftime(buffer, bufsize, format.c_str(), &local_time);
        std::string result(buffer);
        set_env_tz(env_tz);
        return result;
    }

    struct tm strptime(const std::string& format, const std::string& dt_str) const {
#ifndef MOONLIGHT_TZ_UNSAFE
        std::lock_guard<std::mutex> lock(mutex());
#endif
        auto env_tz = get_env_tz();
        set_env_tz(_tz_name);
        struct tm tm_dt;
        ::memset(&tm_dt, 0, sizeof(tm_dt));
        tm_dt.tm_isdst = -1;  // Determine if daylight savings should apply.
        if (::strptime(dt_str.c_str(), format.c_str(), &tm_dt) == nullptr) {
            THROW(core::ValueError, "Date string \"" + dt_str + "\" does not match format \"" + format + ".");
        }
        set_env_tz(env_tz);
        return tm_dt;
    }

private:
    Zone() : _tz_name({}) { }

    static std::optional<std::string> get_env_tz() {
        char* result = getenv("TZ");
        if (result == nullptr) {
            return {};
        }
        return result;
    }

    static void set_env_tz(const std::optional<std::string>& tz_name = {}) {
        if (tz_name.has_value()) {
            std::ostringstream sb;
            sb << "TZ=" << tz_name.value();
            putenv(strdup(sb.str().c_str()));
        } else {
            unsetenv("TZ");
        }
        tzset();
    }

    std::optional<std::string> _tz_name;
};

// --------------------------------------------------------
class Duration {
public:
    Duration() : _ms(0), _bk(breakdown()) { }
    Duration(const Millis& ms) : _ms(ms), _bk(breakdown()) { }
    Duration(const Duration& d) : _ms(d._ms), _bk(breakdown()) { }

    Duration& operator=(const Duration& other) {
        _ms = other._ms;
        return *this;
    }

    struct Breakdown {
        int days;
        int hours;
        int minutes;
        int seconds;
        int millis;
    };

    static Duration zero() {
        return Duration::of_seconds(0);
    }

    static Duration of_days(int days) {
        return Duration(::date::floor<Millis>(::date::days{days}));
    }

    static Duration of_hours(int hours) {
        return Duration(::date::floor<Millis>(std::chrono::hours{hours}));
    }

    static Duration of_minutes(int minutes) {
        return Duration(::date::floor<Millis>(std::chrono::minutes{minutes}));
    }

    static Duration of_seconds(int seconds) {
        return Duration(::date::floor<Millis>(std::chrono::seconds{seconds}));
    }

    int factor() const {
        return _ms < Millis::zero() ? -1 : 1;
    }

    int days() const {
        return factor() * _bk.days;
    }

    int hours() const {
        return factor() * _bk.hours;
    }

    int minutes() const {
        return factor() * _bk.minutes;
    }

    int seconds() const {
        return factor() * _bk.seconds;
    }

    int millis() const {
        return factor() * _bk.millis;
    }

    const Breakdown& bk() const {
        return _bk;
    }

    const Millis& to_chrono_duration() const {
        return _ms;
    }

    Duration operator+(const Duration& d) const {
        return Duration(_ms + d._ms);
    }

    Duration operator-(const Duration& d) const {
        return Duration(_ms - d._ms);
    }

    bool operator==(const Duration& d) const {
        return _ms == d._ms;
    }

    bool operator!=(const Duration& d) const {
        return !(*this == d);
    }

    bool operator>(const Duration& d) const {
        return _ms > d._ms;
    }

    bool operator<(const Duration& d) const {
        return _ms < d._ms;
    }

    bool operator>=(const Duration& d) const {
        return (*this == d) || (*this > d);
    }

    bool operator<=(const Duration& d) const {
        return (*this == d) || (*this < d);
    }

    friend std::ostream& operator<<(std::ostream& out, const Duration& d) {
        out << "Duration<";
        if (d.factor() < 0) {
            out << "-";
        }
        std::ios out_state(nullptr);
        out_state.copyfmt(out);
        out << d.days() << "d"
            << std::setfill('0') << std::setw(2)
            << d.hours() << ":"
            << d.minutes() << ":"
            << d.seconds() << " "
            << std::setw(3)
            << d.millis()
            << ">";
        out.copyfmt(out_state);
        return out;
    }

private:
    Breakdown breakdown() const {
        Millis ms;

        if (_ms < Millis::zero()) {
            ms = ::date::abs(_ms);
        } else {
            ms = _ms;
        }

        auto days = ::date::floor<::date::days>(ms);
        auto hours = ::date::floor<std::chrono::hours>(ms - days);
        auto minutes = ::date::floor<std::chrono::minutes>(ms - days - hours);
        auto seconds = ::date::floor<std::chrono::seconds>(ms - days - hours - minutes);
        auto millis = ::date::floor<Millis>(ms - days - hours - minutes - seconds);
        return {
            .days = factor() * days.count(),
            .hours = factor() * (int)hours.count(),
            .minutes = factor() * (int)minutes.count(),
            .seconds = factor() * (int)seconds.count(),
            .millis = factor() * (int)millis.count()
        };
    }

    Millis _ms;
    Breakdown _bk;
};

// --------------------------------------------------------
inline Duration days(int days) {
    return Duration::of_days(days);
}

inline Duration hours(int hours) {
    return Duration::of_hours(hours);
}

inline Duration minutes(int minutes) {
    return Duration::of_minutes(minutes);
}

inline Duration seconds(int seconds) {
    return Duration::of_seconds(seconds);
}

// --------------------------------------------------------
class Date {
public:
    /**
     * Create a date at the UNIX Epoch.
     */
    Date() : Date(1970, Month::January, 1) { }

    /**
     * Create a date at the start of the given month.
     */
    Date(int year, Month month) : Date(year, month, 1) { }

    /**
     * Throws ValueError if the date is invalid.
     */
    Date(int year, Month month, int day) : _year(year), _month(month), _day(day) {
        validate();
    }

    Date(int year, int month, int day)
    : Date(year, static_cast<Month>(month - 1), day) { }

    Date(const struct tm& tm_dt)
    : Date(tm_dt.tm_year + 1900, tm_dt.tm_mon + 1, tm_dt.tm_mday) { }

    static Date strptime(const std::string& date_str,
                         const std::string& format = DATE_FORMAT) {
        auto tm_date = Zone::utc().strptime(format, date_str);
        if (struct_tm_is_empty(tm_date)) {
            THROW(core::ValueError, "Could not parse Date from \"" + date_str + "\" with format string \"" + format + "\".");
        }
        return Date(tm_date);
    }

    static Date today(const Zone& tz = Zone::utc()) {
        return Date(tz.mk_struct_tm(::date::floor<Millis>(
                    std::chrono::system_clock::now().time_since_epoch())));
    }

    static Date from_isoformat(const std::string& iso_date) {
        return strptime(iso_date, DATE_FORMAT);
    }

    std::string strftime(const std::string& format) const {
        struct tm tm_date;
        load_struct_tm(tm_date);
        tm_date.tm_zone = "UTC";
        tm_date.tm_hour = 0;
        tm_date.tm_min = 0;
        tm_date.tm_sec = 0;
        return Zone::utc().strftime(format, tm_date);
    }

    std::string isoformat() const {
        return strftime(DATE_FORMAT);
    }

    /**
     * Advance forward the given number of calendar days.
     */
    Date advance_days(int days) const {
        Date date = *this;

        for (int x = 0; x < days; x++) {
            if (date == end_of_month()) {
                date = date.next_month();
            } else {
                date = date.with_day(date.day() + 1);
            }
        }

        return date;
    }

    /**
     * Recede backward the given number of calendar days.
     */
    Date recede_days(int days) const {
        Date date = *this;

        for (int x = 0; x < days; x++) {
            if (date == start_of_month()) {
                date = date.prev_month().end_of_month();
            } else {
                date = date.with_day(date.day() - 1);
            }
        }

        return date;
    }

    /**
     * Implements Zeller's rule to determine the weekday.
     */
    Weekday weekday() const {
        int adjustment, mm, yy;
        int y = year();

        if (y < 0) {
            y = 400 - (abs(y) % 400);
        }

        adjustment = (14 - nmonth()) / 12;
        mm = nmonth() + 12 * adjustment - 2;
        yy = y - adjustment;
        return static_cast<Weekday>(
            ((day() + (13 * mm - 1) / 5 +
              yy + yy / 4 - yy / 100 + yy / 400) % 7));
    }

    int nweekday() const {
        return static_cast<int>(weekday());
    }

    int year() const {
        return _year;
    }

    Month month() const {
        return _month;
    }

    int nmonth() const {
        return static_cast<int>(month()) + 1;
    }

    int day() const {
        return _day;
    }

    /**
     * Throws ValueError if the date is invalid.
     */
    Date with_year(int year) const {
        return Date(year, month(), day());
    }

    /**
     * Throws ValueError if the date is invalid.
     */
    Date with_month(Month month) const {
        return Date(year(), month, day());
    }

    /**
     * Throws ValueError if the date is invalid or the month is out of range.
     */
    Date with_nmonth(int nmonth) const {
        if (nmonth < 1 || nmonth > 12) {
            THROW(core::ValueError, "Numeric month value is out of range (1-12)");
            THROW(core::ValueError, "Numeric month value is out of range (1-12).");
        }

        return with_month(static_cast<Month>(nmonth - 1));
    }

    /**
     * Throws ValueError if the date is invalid.
     */
    Date with_day(int day) const {
        return Date(year(), month(), day);
    }

    /**
     * Advance to the first day of this date's next month.
     */
    Date next_month() const {
        Date date = with_day(1);

        if (month() == Month::December) {
            date = date.with_year(date.year() + 1).with_month(Month::January);
        } else {
            date = date.with_nmonth(nmonth() + 1);
        }

        return date;
    }

    /**
     * Recede to the first day of this date's last month.
     */
    Date prev_month() const {
        Date date = with_day(1);

        if (month() == Month::January) {
            date = date.with_year(date.year() - 1).with_month(Month::December);
        } else {
            date = date.with_nmonth(nmonth() - 1);
        }

        return date;
    }

    /**
     * Advance to the last day of this date's month.
     */
    Date end_of_month() const {
        return with_day(last_day_of_month(year(), month()));
    }

    /**
     * Revert back to the first day of this date's month.
     */
    Date start_of_month() const {
        return with_day(1);
    }

    bool operator<(const Date& rhs) const {
        if (year() == rhs.year()) {
            if (month() == rhs.month()) {
                return day() < rhs.day();
            }
            return month() < rhs.month();
        }
        return year() < rhs.year();
    }

    bool operator>(const Date& rhs) const {
        return (! (*this < rhs)) && (! (*this == rhs));
    }

    bool operator<=(const Date& rhs) const {
        return (*this < rhs) || (*this == rhs);
    }

    bool operator>=(const Date& rhs) const {
        return (! (*this < rhs) || (*this == rhs));
    }

    bool operator==(const Date& rhs) const {
        return (year() == rhs.year() &&
                month() == rhs.month() &&
                day() == rhs.day());
    }

    bool operator!=(const Date& rhs) const {
        return ! (*this == rhs);
    }

    Date& operator++() {
        *this = advance_days(1);
        return *this;
    }

    Date operator++(int) {
        Date date = *this;
        *this = advance_days(1);
        return date;
    }

    Date& operator--() {
        *this = recede_days(1);
        return *this;
    }

    Date operator--(int) {
        Date date = *this;
        *this = recede_days(1);
        return date;
    }

    friend std::ostream& operator<<(std::ostream& out, const Date& date) {
        std::ios out_state(nullptr);
        out_state.copyfmt(out);
        out << "Date<" << std::setfill('0')
            << std::setw(4) << date.year() << "-"
            << std::setw(2) << date.nmonth() << "-"
            << date.day() << ">";
        out.copyfmt(out_state);
        return out;
    }

    void load_struct_tm(struct tm& tm) const {
        tm.tm_sec = 0;
        tm.tm_min = 0;
        tm.tm_hour = 0;
        tm.tm_mday = day();
        tm.tm_mon = nmonth() - 1;
        tm.tm_year = year() - 1900;
        tm.tm_wday = nweekday();
        tm.tm_isdst = -1;
    }

private:
    void validate() {
        if (day() < 1) {
            THROW(core::ValueError, "Day is out of range (less than 1).");
        }

        if (day() > last_day_of_month(year(), month())) {
            THROW(core::ValueError, "Day is out of range (greater than last day of month).");
        }
    }

    int _year;
    Month _month;
    int _day;
};

// --------------------------------------------------------
class Time {
public:
    Time() : Time(0, 0) { }

    Time(int hour, int minute) : _minutes(hour * 60 + minute) {
        validate();
    }

    Time(const struct tm& tm_dt)
    : Time(tm_dt.tm_hour, tm_dt.tm_min) { }

    static const Time& end_of_day() {
        static Time value(23, 59);
        return value;
    }

    static const Time& start_of_day() {
        static Time value(0, 0);
        return value;
    }

    int hour() const {
        return _minutes / 60;
    }

    int minute() const {
        return _minutes % 60;
    }

    Time with_hour(int hour) const {
        return Time(hour, minute());
    }

    Time with_minute(int minute) const {
        return Time(hour(), minute);
    }

    bool operator<(const Time& rhs) const {
        return _minutes < rhs._minutes;
    }

    bool operator>(const Time& rhs) const {
        return (! (*this < rhs)) && (! (*this == rhs));
    }

    bool operator<=(const Time& rhs) const {
        return (*this < rhs) || (*this == rhs);
    }

    bool operator>=(const Time& rhs) const {
        return (! (*this < rhs) || (*this == rhs));
    }

    bool operator==(const Time& rhs) const {
        return _minutes == rhs._minutes;
    }

    bool operator!=(const Time& rhs) const {
        return ! (*this == rhs);
    }

    Millis to_chrono_duration() const {
        return ::date::floor<Millis>(std::chrono::minutes{_minutes});
    }

    friend std::ostream& operator<<(std::ostream& out, const Time& time) {
        std::ios out_state(nullptr);
        out_state.copyfmt(out);
        out << "Time<" << std::setw(2) << std::setfill('0')
            << time.hour() << ":" << time.minute() << ">";
        out.copyfmt(out_state);
        return out;
    }

    void load_struct_tm(struct tm& tm) const {
        tm.tm_min = minute();
        tm.tm_hour = hour();
    }

private:
    void validate() {
        if (_minutes < 0 || _minutes >= 60 * 24) {
            THROW(core::ValueError, "Time is out of range.");
        }
    }

    int _minutes;
};

// --------------------------------------------------------
class Datetime {
public:
    Datetime() : _ms(0) { }
    Datetime(const Duration& d) : _ms(d.to_chrono_duration()) { }
    Datetime(const Millis& ms) : _ms(ms) { }
    Datetime(const Zone& tz, const Millis& ms) : _ms(ms), _tz(tz) { }
    Datetime(const std::string& tz_name, const Millis& ms) : Datetime(Zone(tz_name), ms) { }

    Datetime(const Date& date, const Time& time = Time::start_of_day())
    : Datetime(Zone::utc(), date, time) { }

    Datetime(const Zone& tz, const Date& date, const Time& time = Time::start_of_day()) {
        struct tm tm_localtime;
        date.load_struct_tm(tm_localtime);
        time.load_struct_tm(tm_localtime);
        _tz = tz;
        _ms = ::date::floor<Millis>(std::chrono::seconds{tz.mk_timestamp(tm_localtime)});
    }

    Datetime(const std::string& tz_name, const Date& date, const Time& time = Time::start_of_day())
    : Datetime(Zone(tz_name), date, time) { }

    Datetime(const Zone& tz, int year, Month month, int day = 1, int hour = 0, int minute = 0)
    : Datetime(tz, Date(year, month, day), Time(hour, minute)) { }

    Datetime(const std::string& tz_name, int year, Month month, int day = 1, int hour = 0, int minute = 0)
    : Datetime(Zone(tz_name), year, month, day, hour, minute) { }

    Datetime(int year, Month month, int day = 1, int hour = 0, int minute = 0)
    : Datetime(Zone::utc(), year, month, day, hour, minute) { }

    static Datetime min() {
        return Datetime(Millis::min());
    }

    static Datetime max() {
        return Datetime(Millis::max());
    }

    static Datetime now(const Zone& tz = Zone::utc()) {
        return Datetime(tz, ::date::floor<Millis>(
                std::chrono::system_clock::now().time_since_epoch()));
    }

    static Datetime strptime(const std::string& dt_str,
                             const std::string& format = DATETIME_FORMAT,
                             const Zone& tz = Zone::utc()) {
        auto tm_dt = tz.strptime(format, dt_str);
        if (struct_tm_is_empty(tm_dt)) {
            THROW(core::ValueError, "Could not parse Datetime from \"" + dt_str + "\" using format string \"" + format + "\".");
        }
        return Datetime(tz, ::date::floor<Millis>(std::chrono::seconds{tz.mk_timestamp(tm_dt)}));
    }

    static Datetime from_isoformat(const std::string& iso_dt_str) {
        return strptime(iso_dt_str, DATETIME_8601_UTC, Zone::utc());
    }

    std::string format(const std::string& fmt) const {
        return strftime(fmt);
    }

    std::string strftime(const std::string& format) const {
        auto tm_dt = zone().mk_struct_tm(_ms);
        return zone().strftime(format, tm_dt);
    }

    std::string isoformat() const {
        return zone(Zone::utc()).strftime(DATETIME_8601_UTC);
    }

    Date date() const {
        return Date(zone().mk_struct_tm(_ms));
    }

    Time time() const {
        return Time(zone().mk_struct_tm(_ms));
    }

    Datetime zone(const Zone& tz) const {
        return Datetime(tz, _ms);
    }

    Datetime zone(const std::string& tz_name) const {
        return zone(Zone(tz_name));
    }

    const Zone& zone() const {
        return _tz;
    }

    Datetime utc() const {
        return zone(Zone::utc());
    }

    Datetime local() const {
        return zone(Zone::local());
    }

    Datetime operator+(const Duration& d) const {
        return Datetime(_tz, _ms + d.to_chrono_duration());
    }

    Duration operator-(const Datetime& rhs) const {
        return Duration(_ms - rhs._ms);
    }

    Datetime operator-(const Duration& d) const {
        return Datetime(_tz, _ms - d.to_chrono_duration());
    }

    bool is_dst() const {
        return _tz.mk_struct_tm(_ms).tm_isdst;
    }

    struct tm mk_struct_tm() const {
        return zone().mk_struct_tm(_ms);
    }

    bool operator==(const Datetime& rhs) const {
        return _ms == rhs._ms;
    }

    bool operator!=(const Datetime& rhs) const {
        return !(*this == rhs);
    }

    bool operator<(const Datetime& rhs) const {
        return _ms < rhs._ms;
    }

    bool operator>(const Datetime& rhs) const {
        return _ms > rhs._ms;
    }

    bool operator<=(const Datetime& rhs) const {
        return (*this == rhs) || (*this < rhs);
    }

    bool operator>=(const Datetime& rhs) const {
        return (*this == rhs) || (*this > rhs);
    }

    friend std::ostream& operator<<(std::ostream& out, const Datetime& dt) {
        out << dt.strftime(DATETIME_FORMAT);
        return out;
    }

    Millis to_chrono_duration() const {
        return _ms;
    }

private:
    Millis _ms;
    Zone _tz = Zone::utc();
};

// --------------------------------------------------------
class Range {
public:
    Range(const Datetime& start, const Datetime& end) : _start(start), _end(end) {
        validate();
    }

    Range(const Datetime& start, const Duration& duration) {
        if (duration > Duration::zero()) {
            _start = start;
            _end = start + duration;
        } else {
            _start = start + duration;
            _end = start;
        }
    }

    static Range for_days(const Date& date, int days) {
        return Range(Datetime(date), Datetime(date.advance_days(days)));
    }

    const Datetime& start() const {
        return _start;
    }

    const Datetime& end() const {
        return _end;
    }

    Duration duration() const {
        return end() - start();
    }

    bool contains(const Datetime& dt) const {
        return dt >= start() && dt < end();
    }

    bool contains(const Range& other) const {
        return other.start() >= start() && other.end() <= end();
    }

    bool intersects(const Range& other) const {
        return contains(other)
        || (other.start() >= start() && other.start() < end())
        || (other.end() > start() && other.end() < end());
    }

    std::optional<Range> clip_to(const Range& clipping_range) {
        if (! intersects(clipping_range)) {
            return {};
        }

        return Range(std::max(start(), clipping_range.start()),
                     std::min(end(), clipping_range.end()));
    }

    Range zone(const Zone& zone) const {
        return Range(
            start().zone(zone),
            end().zone(zone));
    }

    Range zone(const std::string& tz_name) const {
        return zone(Zone(tz_name));
    }

    bool operator==(const Range& rhs) const {
        return start() == rhs.start() && end() == rhs.end();
    }

    bool operator!=(const Range& rhs) const {
        return ! (*this == rhs);
    }

    friend std::ostream& operator<<(std::ostream& out, const Range& range) {
        out << "Range<" << range.start() << " - " << range.end() << ">";
        return out;
    }

private:
    void validate() {
        if (start() >= end()) {
            THROW(core::ValueError, "End must be after start for Range.");
        }
    }

    Datetime _start;
    Datetime _end;
};

}
}

#endif /* !__MOONLIGHT_DATE_H */
