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

#include "tinyformat/tinyformat.h"
#include "moonlight/generator.h"
#include "moonlight/exceptions.h"
#include "moonlight/maps.h"
#include "date/date.h"
#include <map>
#include <ctime>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <chrono>

#ifndef MOONLIGHT_TZ_UNSAFE
#include <thread>
#endif

namespace moonlight {
namespace date {

const std::string DEFAULT_FORMAT = "%Y-%m-%d %H:%M:%S %Z";
typedef std::chrono::duration<long long, std::milli> Millis;

// --------------------------------------------------------
class ValueError : public moonlight::core::Exception {
    using Exception::Exception;
};

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
            auto env = tfm::format("TZ=%s", tz_name.value());
            putenv(strdup(env.c_str()));
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

    struct Breakdown {
        int days;
        int hours;
        int minutes;
        int seconds;
        int millis;
    };

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
        tfm::format(out, "%dd%02d:%02d:%02d %03d>",
                    abs(d.days()),
                    abs(d.hours()),
                    abs(d.minutes()),
                    abs(d.seconds()),
                    abs(d.millis()));
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
    : Date(year, static_cast<Month>(month), day) { }

    Date(const struct tm& tm_dt)
    : Date(tm_dt.tm_year + 1900, tm_dt.tm_mon, tm_dt.tm_mday) { }

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

        adjustment = (14 - nmonth()) / 12;
        mm = nmonth() + 12 * adjustment - 2;
        yy = year() - adjustment;
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
            throw ValueError("Numeric month value is out of range (1-12).");
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
        tfm::format(out, "Date<%04d-%02d-%02d>",
                    date.year(),
                    date.nmonth(),
                    date.day());
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
            throw ValueError("Day is out of range (less than 1).");
        }

        if (day() > last_day_of_month(year(), month())) {
            throw ValueError("Day is out of range (greater than last day of month).");
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
        tfm::format(out, "Time<%02d:%02d>", time.hour(), time.minute());
        return out;
    }

    void load_struct_tm(struct tm& tm) const {
        tm.tm_min = minute();
        tm.tm_hour = hour();
    }

private:
    void validate() {
        if (_minutes < 0 || _minutes >= 60 * 24) {
            throw ValueError("Time is out of range.");
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

    Datetime(const Date& date, const Time& time = Time::start_of_day())
    : Datetime(Zone::utc(), date, time) { }

    Datetime(const Zone& tz, const Date& date, const Time& time = Time::start_of_day()) {
        struct tm tm_localtime;
        date.load_struct_tm(tm_localtime);
        time.load_struct_tm(tm_localtime);
        _tz = tz;
        _ms = ::date::floor<Millis>(std::chrono::seconds{tz.mk_timestamp(tm_localtime)});
    }

    Datetime(const Zone& tz, int year, Month month, int day = 1, int hour = 0, int minute = 0)
    : Datetime(tz, Date(year, month, day), Time(hour, minute)) { }

    Datetime(int year, Month month, int day = 1, int hour = 0, int minute = 0)
    : Datetime(Zone::utc(), year, month, day, hour, minute) { }

    static Datetime now(const Zone& tz = Zone::utc()) {
        return Datetime(tz, ::date::floor<Millis>(
                std::chrono::system_clock::now().time_since_epoch()));
    }

    Date date() const {
        return Date(_tz.mk_struct_tm(_ms));
    }

    Time time() const {
        return Time(_tz.mk_struct_tm(_ms));
    }

    Datetime zone(const Zone& tz) const {
        return Datetime(tz, _ms);
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

    std::string format(const std::string& fmt = DEFAULT_FORMAT) const {
        return _tz.strftime(fmt, _tz.mk_struct_tm(_ms));
    }

    friend std::ostream& operator<<(std::ostream& out, const Datetime& dt) {
        out << dt.format();
        return out;
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

    Range(const Datetime& start, const Duration& duration) : Range(start, start + duration) { }

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

    bool operator==(const Range& rhs) const {
        return start() == rhs.start() && end() == rhs.end();
    }

    bool operator!=(const Range& rhs) const {
        return ! (*this == rhs);
    }

    friend std::ostream& operator<<(std::ostream& out, const Range& range) {
        tfm::format(out, "Range<%s - %s>", range.start().format(), range.end().format());
        return out;
    }

private:
    void validate() {
        if (start() >= end()) {
            throw ValueError("End must be after start for Range.");
        }
    }

    Datetime _start;
    Datetime _end;
};

}
}

#endif /* !__MOONLIGHT_DATE_H */
