/*
 * log.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday December 19, 2024
 */

#include <iostream>
#include <csignal>
#include "moonlight/test.h"
#include "moonlight/log.h"

using namespace moonlight;
using namespace moonlight::test;
using namespace moonlight::log;

int main() {
    return TestSuite("moonlight log tests")
    .test("test basic logging", []() {
        Logger log = Logger::root();
        log.sync_to(new StreamSync(std::cout));

        log("BasicLoggingTest")
            .with("version", 1)
            .with("result", JSON().with("code", 200).with("message", "OK"))
            .ok();
    })
    .test("test nested loggers", []() {
        auto log = Logger::root();
        log.sync_to(new StreamSync(std::cout));

        log("BasicLoggingTest")
            .with("version", 1)
            .with("result", JSON().with("code", 200).with("message", "OK"))
            .ok();

        auto hello_log = log.logger("Submodule");
        hello_log("HelloEvent").ok();
    })
    .run();
}
