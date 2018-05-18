#include "moonlight/cli.h"
#include "moonlight/test.h"
#include <map>

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
         
         assert_equal(std::string("test"), cmd.get_program_name());
         assert_equal(std::string("1"), cmd.require("b", "bananas"));
         assert_equal(std::string("2"), cmd.require("a", "apples"));
         assert_equal(std::string("1"), cmd.require("b", "bananas"));
         assert_true(cmd.check("v", "verbose"));
         assert_true(!cmd.check("f", "force"));
      })
      .run();
}
