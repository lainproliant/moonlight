/*
 * file.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Friday January 22, 2021
 *
 * Distributed under terms of the MIT license.
 */

#include "moonlight/file.h"
#include "moonlight/test.h"

using namespace moonlight::test;

int main() {
    return TestSuite("moonlight file tests")
    .die_on_signal(SIGSEGV)
    .test("scan_eq and scan_line_eq", []() {
        auto infile = moonlight::file::open_r("test/data/test_file.txt");
        auto input = moonlight::file::BufferedInput(infile);

        ASSERT_EQUAL((char)input.peek(), '*');
        ASSERT(input.scan_eq("***"));
        ASSERT(input.scan_line_eq("***", 3));
        std::string first_line = input.getline();
        std::cout << "first line: {" << first_line << "}" << std::endl;
        ASSERT_EQUAL(first_line, std::string("***asdfghjkl***\n"));
        ASSERT_EQUAL((char)input.peek(), '[');
        ASSERT_FALSE(input.scan_line_eq("]"));
        std::string second_line = input.getline();
        std::cout << "second line: {" << second_line << "}" << std::endl;
        ASSERT_EQUAL(second_line, std::string("[asdfghjkl\n"));
        ASSERT_EQUAL(input.getc(), EOF);
    })
    .run();
}
