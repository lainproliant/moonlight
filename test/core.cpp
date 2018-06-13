#include "moonlight/core.h"
#include "moonlight/test.h"
#include <map>

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
   return TestSuite("moonlight core tests")
      .test("map::keys/map::values test", [&]() {
         map<string, int> M = {
            {"apple", 1},
            {"banana", 2},
            {"orange", 3}};

         assert_true(lists_equal(
            collect::sorted(maps::keys(M)),
            {"apple", "banana", "orange"}));
         assert_true(lists_equal(
            collect::sorted(maps::values(M)),
            {1, 2, 3}));
      })
      .test("mmaps::build static initialization", [&]() {
         auto mmap = mmaps::build<string, string>({
            {"fruit", {"apple", "orange", "banana", "pear"}},
            {"meat", {"beef", "chicken", "pork", "fish", "lamb"}},
            {"drink", {"coffee", "tea", "ice water"}}});

         assert_true(! lists_equal(mmaps::collect(mmap, "fruit"),
                  {"apple", "banana", "pear"}));

         assert_true(lists_equal(mmaps::collect(mmap, "fruit"),
                  {"apple", "orange", "banana", "pear"}));
      })
      .run();
}
