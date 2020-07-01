#include "moonlight/core.h"
#include "moonlight/test.h"
#include <map>
#include <random>

using namespace std;
using namespace moonlight;
using namespace moonlight::test;

int main() {
   return TestSuite("moonlight core tests")
      .test("map::keys/map::values test", []() {
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
      .test("mmap::build static initialization", []() {
         auto mmap = mmap::build<string, string>({
            {"fruit", {"apple", "orange", "banana", "pear"}},
            {"meat", {"beef", "chicken", "pork", "fish", "lamb"}},
            {"drink", {"coffee", "tea", "ice water"}}});

         assert_true(! lists_equal(mmap::collect(mmap, "fruit"),
                  {"apple", "banana", "pear"}));

         assert_true(lists_equal(mmap::collect(mmap, "fruit"),
                  {"apple", "orange", "banana", "pear"}));
      })
      .test("Finalizer tests", []() {
         std::random_device rd;
         std::mt19937 gen(rd());
         std::bernoulli_distribution d(0.5);
         int x = 0;

         if (d(gen)) {
            core::Finalizer finally([&]() {
               x++;
            });
         } else {
            core::Finalizer finally([&]() {
               x++;
            });
         }

         assert_equal(x, (int) 1);
      })
      .test("file::BufferedInput test", []() {
         const std::string input_string = "look it's a bird!";
         std::istringstream in(input_string);
         auto reader = file::BufferedInput(in);

         int c = reader.getc();
         assert_equal(c, (int) 'l', "first character check");

         assert_true(reader.scan_eq("ook it's a bird!"), "first scan check");

         reader.advance(4);

         assert_true(reader.scan_eq("it's"), "second scan check");

         size_t x;
         for (x = 0; reader.getc() != EOF; x++) { }

         assert_equal(x, 12ul, "EOF check");
      })
      .run();
}
