#include <csignal>
#include <iostream>
#include "moonlight/test.h"
#include "moonlight/classify.h"
#include "tinyformat/tinyformat.h"

using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight classify tests")
    .test("Simple example using side effects", []() {
        Classifier<int> classify;
        std::vector<std::string> results;

        classify(1) = [&]() { results.push_back("one"); };
        classify([](int x) { return x % 2 == 0; }) = [&](int x) {
             results.push_back(tfm::format("%s is even", x));
        };
        classify.otherwise() = [&](int x) {
            results.push_back(tfm::format("%d is not even", x));
        };

        classify.apply(2);
        classify.apply(1001);
        classify.apply(1);

        for (auto s : results) {
            std::cout << s << std::endl;
        }

        ASSERT_EQUAL(results, {
            "2 is even",
            "1001 is not even",
            "one"
        });
    })
    .test("simple functional example", []() {
        Classifier<int, std::string> classify;
        classify(0) = []() { return "empty"; };
        classify(1) = []() { return "lonely"; };
        classify(2) = []() { return "a crowd"; };

        const std::vector<int> numbers = {-1, 0, 1, 2, 3};
        std::vector<std::string> results;

        for (auto n : numbers) {
            results.push_back(classify.apply(n).value_or("???"));
        }

        for (auto s : results) {
            std::cout << s << std::endl;
        }

        ASSERT_EQUAL(results, {
            "???",
            "empty",
            "lonely",
            "a crowd",
            "???"
        });
    })
    .run();
}
