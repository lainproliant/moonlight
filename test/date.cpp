#include "moonlight/test.h"
#include "moonlight/date.h"
#include <csignal>
#include <iostream>

using namespace moonlight;
using namespace moonlight::test;
using namespace moonlight::date;

int main() {
    return TestSuite("moonlight date tests")
    .test("last_day_of_month()", [&]() {
        assert_equal(last_day_of_month(2048, Month::February), 29);
        assert_equal(last_day_of_month(2049, Month::February), 28);
        assert_equal(last_day_of_month(2048, Month::November), 30);
        assert_equal(last_day_of_month(2049, Month::December), 31);
    })
    .test("class Duration", [&]() {
        Duration dA = minutes(8670);
        std::cout << "dA = " << dA << std::endl;
        assert_equal(dA.days(), 6, "dA days");
        assert_equal(dA.hours(), 0, "dA hours");
        assert_equal(dA.minutes(), 30, "dA minutes");

        Duration dB = hours(50) + minutes(30);
        std::cout << "dB = "<< dB << std::endl;
        assert_equal(dB.days(), 2, "dB days");
        assert_equal(dB.hours(), 2, "dB hours");
        assert_equal(dB.minutes(), 30, "dB minutes");

        Duration dC = days(21) + hours(22) + minutes(23);
        std::cout << "dC = " << dC << std::endl;
        assert_equal(dC.days(), 21, "dC days");
        assert_equal(dC.hours(), 22, "dC hours");
        assert_equal(dC.minutes(), 23, "dC minutes");

        Duration dD = dA + dB;
        std::cout << "dD = dA + dB = " << dD << std::endl;
        assert_equal(dD.days(), 8, "dD days");
        assert_equal(dD.hours(), 3, "dD hours");
        assert_equal(dD.minutes(), 0, "dD minutes");

        Duration dE = dB + dC;
        std::cout << "dE = dB = dC = " << dE << std::endl;
        assert_equal(dE.days(), 24, "dE days");
        assert_equal(dE.hours(), 0, "dE hours");
        assert_equal(dE.minutes(), 53, "dE minutes");

        Duration dF = dA - dC;
        std::cout << "dF = dA - dC = " << dF << std::endl;
        assert_equal(dF.factor(), -1, "dF factor");
        assert_equal(dF.days(), 15, "dF days");
        assert_equal(dF.hours(), 21, "dF hours");
        assert_equal(dF.minutes(), 53, "dF minutes");
    })
    .test("class Date", [&]() {
        Date dateA;

        std::cout << "dateA = " << dateA << std::endl;
        assert_equal(dateA.year(), 1970, "dateA year");
        assert_equal(dateA.month(), Month::January, "dateA month");
        assert_equal(dateA.day(), 1, "dateA day");

        Date dateB(1988, Month::June, 8);
        std::cout << "dateB = " << dateB << std::endl;
        std::cout << "dateB.weekday() = " << static_cast<int>(dateB.weekday()) << std::endl;
        assert_equal(dateB.year(), 1988, "dateB year");
        assert_equal(dateB.month(), Month::June, "dateB month");
        assert_equal(dateB.day(), 8, "dateB day");
        assert_equal(dateB.weekday(), Weekday::Wednesday, "dateB weekday()");
        assert_equal(dateB.end_of_month(), Date(1988, Month::June, 30), "dateB end_of_month()");
        assert_equal(dateB.start_of_month(), Date(1988, Month::June, 1), "dateB start_of_month()");
        assert_equal(dateB.advance_days(1), Date(1988, Month::June, 9), "dateB advance_days(1)");
        assert_equal(dateB.advance_days(30), Date(1988, Month::July, 8), "dateB advance_days(30)");
        assert_equal(dateB.recede_days(1), Date(1988, Month::June, 7), "dateB recede_days(1)");
        assert_equal(dateB.recede_days(30), Date(1988, Month::May, 9), "dateB recede_days(30)");

        Date dateC(2020, Month::December, 15);
        assert_equal(dateC.weekday(), Weekday::Tuesday, "dateC weekday()");
        assert_equal(dateC.weekday(), Weekday::Tuesday, "dateC weekday()");
        assert_equal(dateC.next_month(), Date(2021, Month::January, 1), "dateC next_month()");
        assert_equal(dateC.prev_month(), Date(2020, Month::November, 1), "dateC prev_month()");
        assert_true(dateA < dateB && dateB < dateC, "dateA < dateB < dateC");
        assert_true(dateC > dateB && dateB > dateA, "dateC > dateB > dateA");

        try {
            Date dateZZ(9595, Month::April, 31);
            assert_true(false, "dateZZ should have thrown ValueError.");

        } catch (const ValueError& e) {
            std::cout << "dateZZ -> Caught expected ValueError." << std::endl;
        }
    })
    .test("class Time", [&]() {
        Time timeA;
        std::cout << "timeA = " << timeA << std::endl;
        assert_equal(timeA.hour(), 0, "timeA hour()");
        assert_equal(timeA.minute(), 0, "timeA minute()");

        Time timeB(23, 59);
        std::cout << "timeB " << timeB << std::endl;
        assert_equal(timeB.hour(), 23, "timeB hour()");
        assert_equal(timeB.minute(), 59, "timeB minute()");

        assert_true(timeA < timeB, "timeA < timeB");
        assert_true(timeB > timeA, "timeB > timeA");
        assert_true(timeB == Time(23, 59), "timeB == Time(23, 59)");

        try {
            Time timeC(99, 99);
            std::cout << "timeC " << timeC << std::endl;
            assert_true(false, "timeC should have thrown ValueError.");

        } catch (const ValueError& e) {
            std::cout << "timeC -> Caught expected ValueError." << std::endl;
        }
    })
    .test("class Datetime", [&]() {
        Datetime dtA;
        std::cout << "dtA = " << dtA << std::endl;
        assert_equal(dtA.date(), Date(1970, Month::January, 1), "dtA date()");
        assert_equal(dtA.time(), Time(0, 0), "dtA time()");

        Datetime dtB(seconds(1608163834));
        std::cout << "dtB = " << dtB << std::endl;
        assert_equal(dtB.date(), Date(2020, Month::December, 17), "dtB date()");
        assert_equal(dtB.time(), Time(0, 10), "dtB time()");

        Duration durationA = hours(-2) - minutes(30);
        std::cout << "durationA = " << durationA << std::endl;
        Datetime dtC = dtB + durationA;
        std::cout << "dtC = dtB + durationA = " << dtC << std::endl;
        assert_equal(dtC.date(), Date(2020, Month::December, 16), "dtC date()");
        assert_equal(dtC.time(), Time(21, 40), "dtC time()");
        Duration durationB = dtC - dtB;
        std::cout << "durationB = dtC - dtB = " << durationB << std::endl;
        assert_equal(durationB, durationA, "durationB == durationA");

        Duration durationC = days(30) + hours(23) + minutes(15);
        std::cout << "durationC = " << durationC << std::endl;
        Datetime dtD = dtB + durationC;
        std::cout << "dtD = dtB + durationC = " << dtD << std::endl;
        assert_equal(dtD.date(), Date(2021, Month::January, 16), "dtD date()");
        assert_equal(dtD.time(), Time(23, 25), "dtD time()");

        Datetime dtE = dtD.local();
        std::cout << "dtE = dtD.local() = " << dtE << std::endl;
        Datetime dtF = dtE.utc();
        std::cout << "dtF = dtE.utc() = " << dtF << std::endl;
        assert_false(dtE != dtF, "dtE != dtF");
        assert_equal(dtD, dtF, "dtD == dtF");

        Datetime nowUTC = Datetime::now().utc();
        std::cout << "nowUTC = " << nowUTC << std::endl;

        Datetime nowLocal = nowUTC.local();
        std::cout << "nowLocal = " << nowLocal << std::endl;
        std::cout << "nowLocal.is_dst() = " << nowLocal.is_dst() << std::endl;
        Duration sixMonths = Duration::of_days(180);
        std::cout << "sixMonths = " << sixMonths << std::endl;
        Datetime futureLocal = nowLocal + sixMonths;
        std::cout << "futureLocal = " << futureLocal << std::endl;
        std::cout << "futureLocal.is_dst() = " << futureLocal.is_dst() << std::endl;
    })
    .test("strftime/strptime", []() {
        const std::string format = "%a %b %e %H:%M:%S %Y";
        Datetime dtA = Datetime("America/Los_Angeles", 2021, Month::September, 4, 12, 25);
        std::string dt_strA = dtA.strftime(format);
        assert_equal(dt_strA, std::string("Sat Sep  4 12:25:00 2021"));

        Datetime dtB = Datetime::strptime(dt_strA, format, dtA.zone());
        std::cout << "dtA = " << dtA << std::endl;
        std::cout << "dtB = " << dtB << std::endl;
        std::cout << "dt_strA = " << dt_strA << std::endl;
        std::cout << "dtA.is_dst() = " << dtA.is_dst() << std::endl;
        std::cout << "dtB.is_dst() = " << dtB.is_dst() << std::endl;
        std::cout << "dtA.mk_struct_tm() = " << dtA.mk_struct_tm() << std::endl;
        std::cout << "dtB.mk_struct_tm() = " << dtB.mk_struct_tm() << std::endl;
        std::cout << "dtA.utc() = " << dtA.utc() << std::endl;
        std::cout << "dtB.utc() = " << dtB.utc() << std::endl;
        assert_equal(dtA, dtB);
    })
    .test("isoformat/from_isoformat", []() {
        Datetime dtA = Datetime("America/New_York", 2021, Month::September, 4, 12, 25);
        std::string dt_strA = dtA.isoformat();
        assert_equal(dt_strA, std::string("2021-09-04T16:25:00Z"));

        Datetime dtB = Datetime::from_isoformat(dt_strA);
        std::cout << dtA << std::endl;
        std::cout << dtB << std::endl;
        assert_equal(dtA, dtB);
    })
    .die_on_signal(SIGSEGV)
    .run();
}
