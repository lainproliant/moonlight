#include <csignal>
#include <iostream>
#include "moonlight/test.h"
#include "moonlight/date.h"

using namespace moonlight;
using namespace moonlight::test;
using namespace moonlight::date;

int main() {
    return TestSuite("moonlight date tests")
    .test("last_day_of_month()", [&]() {
        ASSERT_EQUAL(last_day_of_month(2048, Month::February), 29);
        ASSERT_EQUAL(last_day_of_month(2049, Month::February), 28);
        ASSERT_EQUAL(last_day_of_month(2048, Month::November), 30);
        ASSERT_EQUAL(last_day_of_month(2049, Month::December), 31);
    })
    .test("class Duration", [&]() {
        Duration dA = minutes(8670);
        std::cout << "dA = " << dA << std::endl;
        ASSERT_EQUAL(dA.days(), 6);
        ASSERT_EQUAL(dA.hours(), 0);
        ASSERT_EQUAL(dA.minutes(), 30);

        Duration dB = hours(50) + minutes(30);
        std::cout << "dB = "<< dB << std::endl;
        ASSERT_EQUAL(dB.days(), 2);
        ASSERT_EQUAL(dB.hours(), 2);
        ASSERT_EQUAL(dB.minutes(), 30);

        Duration dC = days(21) + hours(22) + minutes(23);
        std::cout << "dC = " << dC << std::endl;
        ASSERT_EQUAL(dC.days(), 21);
        ASSERT_EQUAL(dC.hours(), 22);
        ASSERT_EQUAL(dC.minutes(), 23);

        Duration dD = dA + dB;
        std::cout << "dD = dA + dB = " << dD << std::endl;
        ASSERT_EQUAL(dD.days(), 8);
        ASSERT_EQUAL(dD.hours(), 3);
        ASSERT_EQUAL(dD.minutes(), 0);

        Duration dE = dB + dC;
        std::cout << "dE = dB = dC = " << dE << std::endl;
        ASSERT_EQUAL(dE.days(), 24);
        ASSERT_EQUAL(dE.hours(), 0);
        ASSERT_EQUAL(dE.minutes(), 53);

        Duration dF = dA - dC;
        std::cout << "dF = dA - dC = " << dF << std::endl;
        ASSERT_EQUAL(dF.factor(), -1);
        ASSERT_EQUAL(dF.days(), 15);
        ASSERT_EQUAL(dF.hours(), 21);
        ASSERT_EQUAL(dF.minutes(), 53);
    })
    .test("class Date", [&]() {
        Date dateA;

        std::cout << "dateA = " << dateA << std::endl;
        ASSERT_EQUAL(dateA.year(), 1970);
        ASSERT_EQUAL(dateA.month(), Month::January);
        ASSERT_EQUAL(dateA.day(), 1);

        Date dateB(1988, Month::June, 8);
        std::cout << "dateB = " << dateB << std::endl;
        std::cout << "dateB.weekday() = " << static_cast<int>(dateB.weekday()) << std::endl;
        ASSERT_EQUAL(dateB.year(), 1988);
        ASSERT_EQUAL(dateB.month(), Month::June);
        ASSERT_EQUAL(dateB.day(), 8);
        ASSERT_EQUAL(dateB.weekday(), Weekday::Wednesday);
        ASSERT_EQUAL(dateB.end_of_month(), Date(1988, Month::June, 30));
        ASSERT_EQUAL(dateB.start_of_month(), Date(1988, Month::June, 1));
        ASSERT_EQUAL(dateB.advance_days(1), Date(1988, Month::June, 9));
        ASSERT_EQUAL(dateB.advance_days(30), Date(1988, Month::July, 8));
        ASSERT_EQUAL(dateB.recede_days(1), Date(1988, Month::June, 7));
        ASSERT_EQUAL(dateB.recede_days(30), Date(1988, Month::May, 9));

        Date dateC(2020, Month::December, 15);
        ASSERT_EQUAL(dateC.weekday(), Weekday::Tuesday);
        ASSERT_EQUAL(dateC.weekday(), Weekday::Tuesday);
        ASSERT_EQUAL(dateC.next_month(), Date(2021, Month::January, 1));
        ASSERT_EQUAL(dateC.prev_month(), Date(2020, Month::November, 1));
        ASSERT_TRUE(dateA < dateB && dateB < dateC);
        ASSERT_TRUE(dateC > dateB && dateB > dateA);

        try {
            Date dateZZ(9595, Month::April, 31);
            FAIL("dateZZ should have thrown ValueError.");

        } catch (const moonlight::core::ValueError& e) {
            std::cout << "dateZZ -> Caught expected ValueError." << std::endl;
        }
    })
    .test("class Time", [&]() {
        Time timeA;
        std::cout << "timeA = " << timeA << std::endl;
        ASSERT_EQUAL(timeA.hour(), 0);
        ASSERT_EQUAL(timeA.minute(), 0);

        Time timeB(23, 59);
        std::cout << "timeB " << timeB << std::endl;
        ASSERT_EQUAL(timeB.hour(), 23);
        ASSERT_EQUAL(timeB.minute(), 59);

        ASSERT_TRUE(timeA < timeB);
        ASSERT_TRUE(timeB > timeA);
        ASSERT_TRUE(timeB == Time(23, 59));

        try {
            Time timeC(99, 99);
            std::cout << "timeC " << timeC << std::endl;
            FAIL("timeC should have thrown ValueError.");

        } catch (const moonlight::core::ValueError& e) {
            std::cout << "timeC -> Caught expected ValueError." << std::endl;
        }
    })
    .test("class Datetime", [&]() {
        Datetime dtA;
        std::cout << "dtA = " << dtA << std::endl;
        ASSERT_EQUAL(dtA.date(), Date(1970, Month::January, 1));
        ASSERT_EQUAL(dtA.time(), Time(0, 0));

        Datetime dtB(seconds(1608163834));
        std::cout << "dtB = " << dtB << std::endl;
        ASSERT_EQUAL(dtB.date(), Date(2020, Month::December, 17));
        ASSERT_EQUAL(dtB.time(), Time(0, 10));

        Duration durationA = hours(-2) - minutes(30);
        std::cout << "durationA = " << durationA << std::endl;
        Datetime dtC = dtB + durationA;
        std::cout << "dtC = dtB + durationA = " << dtC << std::endl;
        ASSERT_EQUAL(dtC.date(), Date(2020, Month::December, 16));
        ASSERT_EQUAL(dtC.time(), Time(21, 40));
        Duration durationB = dtC - dtB;
        std::cout << "durationB = dtC - dtB = " << durationB << std::endl;
        ASSERT_EQUAL(durationB, durationA);

        Duration durationC = days(30) + hours(23) + minutes(15);
        std::cout << "durationC = " << durationC << std::endl;
        Datetime dtD = dtB + durationC;
        std::cout << "dtD = dtB + durationC = " << dtD << std::endl;
        ASSERT_EQUAL(dtD.date(), Date(2021, Month::January, 16));
        ASSERT_EQUAL(dtD.time(), Time(23, 25));

        Datetime dtE = dtD.local();
        std::cout << "dtE = dtD.local() = " << dtE << std::endl;
        Datetime dtF = dtE.utc();
        std::cout << "dtF = dtE.utc() = " << dtF << std::endl;
        ASSERT_FALSE(dtE != dtF);
        ASSERT_EQUAL(dtD, dtF);

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
    .test("Datetime strftime/strptime", []() {
        const std::string DT_FORMAT = "%a %b %e %H:%M:%S %Y";
        Datetime dtA = Datetime("America/Los_Angeles", 2021, Month::September, 4, 12, 25);
        std::string dt_strA = dtA.strftime(DT_FORMAT);
        ASSERT_EQUAL(dt_strA, std::string("Sat Sep  4 12:25:00 2021"));

        Datetime dtB = Datetime::strptime(dt_strA, DT_FORMAT, dtA.zone());
        std::cout << "dtA = " << dtA << std::endl;
        std::cout << "dtB = " << dtB << std::endl;
        std::cout << "dt_strA = " << dt_strA << std::endl;
        std::cout << "dtA.is_dst() = " << dtA.is_dst() << std::endl;
        std::cout << "dtB.is_dst() = " << dtB.is_dst() << std::endl;
        std::cout << "dtA.mk_struct_tm() = " << dtA.mk_struct_tm() << std::endl;
        std::cout << "dtB.mk_struct_tm() = " << dtB.mk_struct_tm() << std::endl;
        std::cout << "dtA.utc() = " << dtA.utc() << std::endl;
        std::cout << "dtB.utc() = " << dtB.utc() << std::endl;
        ASSERT_EQUAL(dtA, dtB);
    })
    .test("Date strftime/strptime", []() {
        const std::string DATE_FORMAT = "%a %b %e %Y";
        Date dateA = Date::strptime("Mon Jan 3 2022", DATE_FORMAT);
        ASSERT_EQUAL(dateA, Date(2022, Month::January, 3));
        std::cout << dateA.isoformat() << std::endl;
        ASSERT_EQUAL(dateA.isoformat(), std::string("2022-01-03"));
    })
    .test("isoformat/from_isoformat", []() {
        Datetime dtA = Datetime("America/New_York", 2021, Month::September, 4, 12, 25);
        std::string dt_strA = dtA.isoformat();
        ASSERT_EQUAL(dt_strA, std::string("2021-09-04T16:25:00Z"));

        Datetime dtB = Datetime::from_isoformat(dt_strA);
        std::cout << dtA << std::endl;
        std::cout << dtB << std::endl;
        ASSERT_EQUAL(dtA, dtB);
    })
    .die_on_signal(SIGSEGV)
    .run();
}
