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
        auto infile = moonlight::file::open_r("data/test_file.txt");
        auto input = moonlight::file::BufferedInput(infile);

        assert_equal((char)input.peek(), '*', "peek first asterisk");
        assert_true(input.scan_eq("***"), "scan_eq triple asterisk");
        assert_true(input.scan_line_eq("***", 3), "scan_line_eq triple asterisk");
        std::string first_line = input.getline();
        std::cout << "first line: {" << first_line << "}" << std::endl;
        assert_equal(first_line, std::string("***asdfghjkl***\n"), "first line equivalence");
        assert_equal((char)input.peek(), '[', "peek open bracket");
        assert_false(input.scan_line_eq("]"), "peek close bracket");
        std::string second_line = input.getline();
        std::cout << "second line: {" << second_line << "}" << std::endl;
        assert_equal(second_line, std::string("[asdfghjkl\n"), "second line equivalence");
        assert_equal(input.getc(), EOF, "end of file");
    })
    .run();
}
