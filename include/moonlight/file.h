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

#include <fstream>
#include <cstring>
#include <string>

#include "moonlight/exceptions.h"

namespace moonlight {
namespace file {

// ------------------------------------------------------------------
struct Location {
    unsigned int line = 1;
    unsigned int col = 1;
    unsigned int offset = 0;
    std::string name = "";

    static Location nowhere() {
        return {
            0, 0
        };
    }

    friend std::ostream& operator<<(std::ostream& out, const Location& loc) {
        out << "<";
        if (loc.name != "") {
            out << "\'" << loc.name << "\' ";
        }
        out << "L" << loc.line << ":" << loc.col << " +" << loc.offset << ">";
        return out;
    }
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
        THROW(core::RuntimeError, sb.str());
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
        THROW(core::RuntimeError, sb.str());
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
        THROW(core::RuntimeError, sb.str());
    }
}

//-------------------------------------------------------------------
inline std::string tempfile_name(const std::string& prefix = "") {
    char* name_c = ::tempnam(std::filesystem::temp_directory_path().c_str(), prefix.c_str());
    std::string name(name_c);
    ::free(name_c);
    return name;
}

//-------------------------------------------------------------------
class TemporaryFile {
public:
    TemporaryFile(const std::string& prefix, std::ios::openmode mode = std::ios::in | std::ios::out)
    : _filename(tempfile_name(prefix)) {
        open_w(_filename).close();
        _stream = open_rw(_filename, mode);
    }

    ~TemporaryFile() {
        _stream.close();

        if (_cleanup) {
            std::filesystem::remove(_filename);
        }
    }

    TemporaryFile& keep() {
        _cleanup = false;
        return *this;
    }

    const std::filesystem::path& name() const {
        return _filename;
    }

    std::fstream& stream() {
        return _stream;
    }

    std::istream& input() {
        return _stream;
    }

    std::ostream& output() {
        return _stream;
    }

private:
    std::fstream _stream;
    std::filesystem::path _filename;
    bool _cleanup = true;
};

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
     explicit BufferedInput(std::istream& input, const std::string& name = "")
     : _input(input) {
         _loc.name = name;
     }

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

     std::string getline() {
         std::string line;
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
         return _loc.name;
     }

     int line() const {
         return _loc.line;
     }

     int col() const {
         return _loc.col;
     }

     const Location& location() const {
         return _loc;
     }

 private:
     std::istream& _input;
     Location _loc;
     bool _exhausted = false;
     std::string _buffer;
};

}  // namespace file
}  // namespace moonlight


#endif /* !__MOONLIGHT_FILE_H */
