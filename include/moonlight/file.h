/*
 * file.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_FILE_H
#define __MOONLIGHT_FILE_H

#include "moonlight/exceptions.h"
#include <fstream>
#include <cstring>

namespace moonlight {
namespace file {

//-------------------------------------------------------------------
class FileException : public core::Exception {
   using core::Exception::Exception;
};

//-------------------------------------------------------------------
inline std::ifstream open_r(const std::string& filename,
                            std::ios::openmode mode = std::ios::in) {
   try {
      std::ifstream infile;
      infile.exceptions(std::ios::failbit);
      infile.open(filename, mode);
      infile.exceptions(std::ios::badbit);
      return infile;

   } catch (std::exception& e) {
      std::ostringstream sb;
      sb << "Cannot open file " << filename << " for reading: "
      << strerror(errno);
      throw FileException(sb.str());
   }
}

//-------------------------------------------------------------------
inline std::ofstream open_w(const std::string& filename,
                            std::ios::openmode = std::ios::out) {
   try {
      std::ofstream outfile;
      outfile.exceptions(std::ios::failbit);
      outfile.open(filename);
      outfile.exceptions(std::ios::badbit);
      return outfile;

   } catch (std::exception& e) {
      std::ostringstream sb;
      sb << "Cannot open file " << filename << " for writing: "
      << strerror(errno);
      throw FileException(sb.str());
   }
}

//-------------------------------------------------------------------
inline std::fstream open_rw(const std::string& filename,
                            std::ios::openmode mode = std::ios::in | std::ios::out) {
   try {
      std::fstream outfile;
      outfile.exceptions(std::ios::failbit);
      outfile.open(filename, mode);
      outfile.exceptions(std::ios::badbit);
      return outfile;

   } catch (std::exception& e) {
      std::ostringstream sb;
      sb << "Cannot open file " << filename << " for reading and writing: "
      << strerror(errno);
      throw FileException(sb.str());
   }
}

//-------------------------------------------------------------------
inline std::string to_string(std::istream& infile) {
   return std::string(std::istreambuf_iterator<char>(infile), {});
}

//-------------------------------------------------------------------
inline std::string to_string(const std::string& filename) {
   auto infile = open_r(filename);
   return to_string(infile);
}

//-------------------------------------------------------------------
// (namely an alias)
inline std::string slurp(const std::string& filename) {
   return to_string(filename);
}

//-------------------------------------------------------------------
inline void dump(const std::string& filename, const std::string& str) {
   auto outfile = open_w(filename);
   outfile << str;
}

// ------------------------------------------------------------------
class BufferedInput {
public:
    BufferedInput(std::istream& input, const std::string& name = "<input>")
    : _input(input), _name(name) { }

    int getc() {
        int c;

        if (_buffer.size() > 0) {
            c = _buffer[0];
            for (size_t x = 1; x < _buffer.size(); x++) {
               _buffer[x-1] = _buffer[x];
            }
            _buffer.pop_back();

        } else {
            c = _input.get();
        }

        if (c == '\n') {
            _line ++;
            _col = 1;

        } else if (c == EOF) {
            _exhausted = true;

        } else {
            _col ++;
        }

        return c;
    }

    std::string getline() {
        std::string line;
        for (int x = 1; peek(x) != '\n' && peek(x) != EOF; x++) {
            line.push_back(peek(x));
        }
        advance(line.size());
        if (peek() == '\n') {
            line.push_back(getc());
        }
        return line;
    }

    bool is_exhausted() const {
        return _exhausted;
    }

    int peek(size_t offset = 1) {
        if (offset == 0) {
            return EOF;
        }

        while (_buffer.size() < offset) {
            int c = _input.get();
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

    bool scan_eq(const std::string& target, size_t start_at = 0) {
       for (size_t x = 0; x < target.size(); x++) {
          if (peek(start_at + x + 1) != target[x]) {
             return false;
          }
       }

       return true;
    }

    bool scan_line_eq(const std::string& target, size_t start_at = 0, const std::string& escapes = "") {
        for (size_t x = start_at; peek(x) != '\n' && peek(x) != EOF; x++) {
            if (escapes.find(peek(x)) != std::string::npos) {
                x++;
            } else if (scan_eq(target, x)) {
                return true;
            }
        }

        return false;
    }

    bool scan_eq_advance(const std::string& target) {
       if (scan_eq(target)) {
          advance(target.size());
          return true;
       } else {
          return false;
       }
    }

    std::string scan_dump() {
       std::string dump;
       for (size_t x = 1; peek(x) != EOF; x++) {
          dump.push_back(peek(x));
       }
       return dump;
    }

    const std::string& name() const {
        return _name;
    }

    int line() const {
        return _line;
    }

    int col() const {
        return _col;
    }

private:
    std::istream& _input;
    const std::string _name;
    int _line = 1, _col = 1;
    bool _exhausted = false;
    std::string _buffer;
};

}
}


#endif /* !__MOONLIGHT_FILE_H */
