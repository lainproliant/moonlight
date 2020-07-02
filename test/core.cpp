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
         assert_true(lists_equal(str::split(strA, ":"), {"a", "b"}), strA);
         std::cout << strB << " >> " << str::join(str::split(strB, ":"), ",") << std::endl;
         assert_true(lists_equal(str::split(strB, ":"), {"", "b"}), strB);
         std::cout << strC << " >> " << str::join(str::split(strC, ":"), ",") << std::endl;
         assert_true(lists_equal(str::split(strC, ":"), {"a", ""}), strC);
         std::cout << strD << " >> " << str::join(str::split(strD, ":"), ",") << std::endl;
         assert_true(lists_equal(str::split(strD, ":"), {"a"}), strD);
         std::cout << strE << " >> " << str::join(str::split(strE, "//"), ",") << std::endl;
         assert_true(lists_equal(str::split(strE, "//"), {"a", "b", "c"}), strE);
         std::cout << strF << " >> " << str::join(str::split(strF, "//"), ",") << std::endl;
         assert_true(lists_equal(str::split(strF, "//"), {"", "b", "c"}), strF);
         std::cout << strG << " >> " << str::join(str::split(strG, "//"), ",") << std::endl;
         assert_true(lists_equal(str::split(strG, "//"), {"a", "", "c"}), strG);
         std::cout << strH << " >> " << str::join(str::split(strH, "//"), ",") << std::endl;
         assert_true(lists_equal(str::split(strH, "//"), {"a", "b", ""}), strH);
         std::cout << strI << " >> " << str::join(str::split(strI, "//"), ",") << std::endl;
         assert_true(lists_equal(str::split(strI, "//"), {"", "b", ""}), strI);
         std::cout << strJ << " >> " << str::join(str::split(strJ, ":"), ",") << std::endl;
         assert_true(lists_equal(str::split(strJ, ":"), {"a", "", "b", "", ""}), strJ);
         std::cout << strK << " >> " << str::join(str::split(strK, ":"), ",") << std::endl;
         assert_true(lists_equal(str::split(strK, ":"), {"", ""}), strK);
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
