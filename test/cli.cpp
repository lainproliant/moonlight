#include <map>
#include "moonlight/cli.h"
#include "moonlight/test.h"

using namespace moonlight;
using namespace moonlight::test;

int main() {
    return TestSuite("moonlight cli tests")
    .test("command line parsing test", [&]() {
        std::vector<std::string> argv = {
            "test", "-a", "2", "-b1", "--verbose", "oranges" };

        auto cmd = cli::parse(argv,
                              { "verbose", "v" },
                              { "a", "apples", "b", "bananas" });

        ASSERT_EQUAL(std::string("test"), cmd.get_program_name());
        ASSERT_EQUAL(std::string("1"), cmd.require("b", "bananas"));
        ASSERT_EQUAL(std::string("2"), cmd.require("a", "apples"));
        ASSERT_EQUAL(std::string("1"), cmd.require("b", "bananas"));
        ASSERT(cmd.check("v", "verbose"));
        ASSERT_FALSE(cmd.check("f", "force"));
        ASSERT_EQUAL(cmd.args(), {"oranges"});
    })
    .run();
}
