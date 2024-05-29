/*
 * utf8.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday May 28, 2024
 */

#ifndef __UTF8_H
#define __UTF8_H

#include "moonlight/file.h"
#include "utfcpp/source/utf8.h"
#include "tinyformat/tinyformat.h"

namespace moonlight {

namespace unicode {

typedef utf8::utfchar32_t u32_t;
typedef std::basic_string<u32_t> string;

class UnicodeError : public core::Exception {
 public:
     UnicodeError(const std::string& msg, const file::Location& loc, debug::Source where = {}, const std::string& name = moonlight::type_name<UnicodeError>()) :
     core::Exception(create_message(msg, loc), where, name), _loc(loc) { }

     static std::string create_message(const std::string& msg, const file::Location& loc) {
         return tfm::format("%s (%s)", msg, loc);
     }

     const file::Location& loc() const {
         return _loc;
     }

 private:
     const file::Location& _loc;
};

class BufferedInput {
 public:
     explicit BufferedInput(std::istream& input, const std::string& name = "") {
         _loc.name = name;
         _iter = std::istream_iterator<unsigned char>(input);
     }

     u32_t getc() {
         u32_t c;

         if (_buffer.size() > 0) {
             c = _buffer.front();
             _buffer.pop_front();

         } else {
             c = _next();
         }

         if (c == EOF) {
             _exhausted = true;
} else {
             _loc.offset ++;

             if (c == '\n') {
                 _loc.line ++;
                 _loc.col = 1;
             } else {
                 _loc.col ++;
             }
         }

         return c;
     }

     string getline() {
         string line;

         for (int x = 1; peek(x) != '\n' && peek(x) != EOF; x++) {
             line.push_back(peek(x));
         }
         advance(line.size());
         if (peek() == EOF) {
             _exhausted = true;
         } else if (peek() == '\n') {
             line.push_back(getc());
         }
         return line;
     }

     bool is_exhausted() const {
         return _exhausted;
     }

     u32_t peek(size_t offset = 1) {
         if (offset == 0) {
             return EOF;
         }

         while (_buffer.size() < offset) {
             int c = _next();
             if (c == EOF) {
                 return EOF;
             }
             _buffer.push_back(c);
         }

         return _buffer[offset-1];
     }

     void advance(size_t offset = 1) {
         for (size_t x = 0; x < offset; x++) {
             getc();
         }
     }

     bool scan_eq(const string& target, size_t start_at = 0) {
         for (size_t x = 0; x < target.size(); x++) {
             if (peek(start_at + x + 1) != target[x]) {
                 return false;
             }
         }

         return true;
     }

     bool scan_line_eq(const string& target, size_t start_at = 0, const string& escapes = U"") {
         for (size_t x = start_at; peek(x) != '\n' && peek(x) != EOF; x++) {
             if (escapes.find(peek(x)) != string::npos) {
                 x++;
             } else if (scan_eq(target, x)) {
                 return true;
             }
         }

         return false;
     }

     bool scan_eq_advance(const string& target) {
         if (scan_eq(target)) {
             advance(target.size());
             return true;
         } else {
             return false;
         }
     }

     string scan_dump() {
         string dump;
         for (size_t x = 1; peek(x) != EOF; x++) {
             dump.push_back(peek(x));
         }
         return dump;
     }

     const std::string& name() const {
         return _loc.name;
     }

     int line() const {
         return _loc.line;
     }

     int col() const {
         return _loc.col;
     }

     const file::Location& location() const {
         return _loc;
     }

 private:
     u32_t _next() {
         try {
             return utf8::next(_iter, _end);

         } catch (const utf8::not_enough_room& e) {
             return EOF;
         } catch (const utf8::invalid_utf8& e) {
             THROW(UnicodeError, "Invalid utf-8 sequence.", _loc);
         } catch (const utf8::invalid_code_point& e) {
             THROW(UnicodeError, "Invalid unicode codepoint.", _loc);
         }
     }

     std::istream_iterator<unsigned char> _iter;
     std::istream_iterator<unsigned char> _end;
     file::Location _loc;
     bool _exhausted = false;
     std::deque<u32_t> _buffer;
};

}
}

#endif /* !__UTF8_H */
