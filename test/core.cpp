#include "moonlight/test.h"
#include "moonlight/file.h"
#include "moonlight/maps.h"
#include "moonlight/mmap.h"
#include <map>
#include <random>
#include <string>

using namespace moonlight;
using namespace moonlight::test;

int main() {
   return TestSuite("moonlight core tests")
      .test("map::keys/map::values test", []() {
          std::map<std::string, int> M = {
            {"apple", 1},
            {"banana", 2},
            {"orange", 3}};

         ASSERT_EQUAL(
            maps::keys(M).sorted().collect(),
            {"apple", "banana", "orange"});
         ASSERT_EQUAL(
            maps::values(M).sorted().collect(),
            {1, 2, 3});
      })
      .test("mmap::build static initialization", []() {
         auto mmap = mmap::build<std::string, std::string>({
            {"fruit", {"apple", "orange", "banana", "pear"}},
            {"meat", {"beef", "chicken", "pork", "fish", "lamb"}},
            {"drink", {"coffee", "tea", "ice water"}}});

         ASSERT_EQUAL(mmap::collect(mmap, "fruit"),
                  {"apple", "orange", "banana", "pear"});
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

         ASSERT_EQUAL(x, (int) 1);
      })
      .test("str::split test", []() {
         const std::string strA = "a:b";
         const std::string strB = ":b";
         const std::string strC = "a:";
         const std::string strD = "a";

         const std::string strE = "a//b//c";
         const std::string strF = "//b//c";
         const std::string strG = "a////c";
         const std::string strH = "a//b//";
         const std::string strI = "//b//";

         const std::string strJ = "a::b::";
         const std::string strK = ":";

         std::cout << strA << " >> " << str::join(str::split(strA, ":"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strA, ":"), {"a", "b"});
         std::cout << strB << " >> " << str::join(str::split(strB, ":"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strB, ":"), {"", "b"});
         std::cout << strC << " >> " << str::join(str::split(strC, ":"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strC, ":"), {"a", ""});
         std::cout << strD << " >> " << str::join(str::split(strD, ":"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strD, ":"), {"a"});
         std::cout << strE << " >> " << str::join(str::split(strE, "//"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strE, "//"), {"a", "b", "c"});
         std::cout << strF << " >> " << str::join(str::split(strF, "//"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strF, "//"), {"", "b", "c"});
         std::cout << strG << " >> " << str::join(str::split(strG, "//"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strG, "//"), {"a", "", "c"});
         std::cout << strH << " >> " << str::join(str::split(strH, "//"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strH, "//"), {"a", "b", ""});
         std::cout << strI << " >> " << str::join(str::split(strI, "//"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strI, "//"), {"", "b", ""});
         std::cout << strJ << " >> " << str::join(str::split(strJ, ":"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strJ, ":"), {"a", "", "b", "", ""});
         std::cout << strK << " >> " << str::join(str::split(strK, ":"), ",") << std::endl;
         ASSERT_EQUAL(str::split(strK, ":"), {"", ""});
      })
      .test("file::BufferedInput test", []() {
         const std::string input_string = "look it's a bird!";
         std::istringstream in(input_string);
         auto reader = file::BufferedInput(in);

         int c = reader.getc();
         ASSERT_EQUAL(c, (int) 'l');

         ASSERT(reader.scan_eq("ook it's a bird!"));

         reader.advance(4);

         ASSERT(reader.scan_eq("it's"));

         size_t x;
         for (x = 0; reader.getc() != EOF; x++) { }

         ASSERT_EQUAL(x, 12ul);
      })
      .run();
}
