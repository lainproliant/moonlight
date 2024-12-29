/*
 * ## file.h: Convenient wrappers for interacting with files. -------
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Tuesday June 30, 2020
 *
 * Distributed under terms of the MIT license.
 *
 * ## Usage ---------------------------------------------------------
 * This library offers a variety of useful wrapper functions and classes for
 * interacting with file IO, temporary files, and buffered input.
 *
 * - `file::Location`: A structure to represent a line, column, and byte offset
 *   in a file, buffer, or input stream.  Extremely useful in parsing contexts.
 * - `file::open_r(name)`: Opens a file in read-only mode.  Throws a
 *   `core::RuntimeError` if the file can't be opened for reading.
 * - `file::open_w(name)`: Opens a file in write-only mode.  Throws a
 *   `core::RuntimeError` if the file can't be opened for writing.
 * - `file::open_rw(name)`: Opens a file in read-write mode.  Throws a
 *   `core::RuntimeError` if the file can't be opened for simultaneous reading
 *   and writing.
 * - `file::tempfile_name(prefix="", suffix="")`: Constructs a temporary
 *   filename path using the given prefix and suffix (optional), prepended with
 *   the system's temporary file path as provided by
 *   `std::filesystem::temp_directory_path()`.
 * - `TemporaryFile`: An RAII wrapper for creating a temporary file using
 *   `file::tempfile_name()`.  Opens the file in read-write mode by default, and
 *   deletes the temporary file when the object leaves scope, unless `keep()` is
 *   called to prevent this.
 * - `file::to_string(v)`: Reads the full contents of an input file into an
 *   `std::string`.  `v` can be either an `std::istream&` or a filename which is
 *   then opened to retrieve its contents and subsequently closed.
 * - `slurp(name)`: An alias for loading the contents of a file as an
 *   `std::string`.
 * - `dump(name, s)`: Writes the given string `s` to the named file.  Will
 *   overwrite any existing contents in the named file if it exists.
 * - `BufferedInput`: A useful wrapper around an input stream, providing
 *   automatically buffered input, look-ahead, and scanning capabilities.
 *   Provides the current location in the output stream as a `file::Location`
 *   via `location()`.  Extremely useful in the construction of look-ahead
 *   parsers that wish to read from an input stream rather than a byte buffer.
 */
#ifndef __MOONLIGHT_FILE_H
#define __MOONLIGHT_FILE_H

#include <fstream>
#include <cstring>
#include <string>
#include <deque>

#include "moonlight/exceptions.h"
#include "moonlight/nanoid.h"

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
inline std::string tempfile_name(const std::string& prefix = "", const std::string& suffix = "", int length = 10) {
    return std::filesystem::temp_directory_path() / (prefix + nanoid::generate(length) + suffix);
}

//-------------------------------------------------------------------
class TemporaryFile {
public:
    TemporaryFile(const std::string& prefix, const std::string& suffix, std::ios::openmode mode = std::ios::in | std::ios::out)
    : _filename(tempfile_name(prefix, suffix)) {
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
             c = _buffer.front();
             _buffer.pop_front();

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
     std::deque<int> _buffer;
};

}  // namespace file
}  // namespace moonlight


#endif /* !__MOONLIGHT_FILE_H */
