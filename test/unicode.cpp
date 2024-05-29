/*
 * utf8.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 28, 2024
 */

#include "moonlight/unicode.h"
#include "moonlight/test.h"

using namespace moonlight::test;
namespace unicode = moonlight::unicode;

int main() {
    return TestSuite("moonlight utf8 tests")
    .die_on_signal(SIGSEGV)
    .test("read valid utf-8 until end of file", []() {
        const std::string sample_text = "間濾mew";

        std::istringstream infile(sample_text);
        unicode::BufferedInput input(infile);
        unicode::string result32;

        while (! input.is_exhausted()) {
            unicode::u32_t c = input.getc();
            if (c != EOF) {
                result32.push_back(c);
                std::cout << utf8::utf32to8(result32) << std::endl;
            }
        }

        std::string result = utf8::utf32to8(result32);
        ASSERT_EQUAL(sample_text, result);
    })
    .run();
}
