/*
 * debug.h
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Thursday January 13, 2022
 *
 * Distributed under terms of the MIT license.
 */

#ifndef __MOONLIGHT_DEBUG_H
#define __MOONLIGHT_DEBUG_H

#include <unistd.h>
#include <execinfo.h>
#include <cstring>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <cxxabi.h>

#include "moonlight/constants.h"
#include "moonlight/collect.h"
#include "moonlight/finally.h"
#include "moonlight/string.h"

#define LOCATION moonlight::debug::Source(__FILE__, __PRETTY_FUNCTION__, __LINE__)

namespace moonlight {
namespace debug {

namespace fs = std::filesystem;

/**
 * @note Don't use this method, use `sys::check()` instead.
 */
inline bool check(const std::string& command, std::string& output,
                  int& returncode) noexcept {
    char line_buffer[MOONLIGHT_SYS_CHECK_BUFSIZE];
    std::ostringstream sb;
    FILE* pipe = popen(command.c_str(), "r");

    core::Finalizer finally([&]() {
        if (pipe) {
            returncode = pclose(pipe);
        }
    });

    if (! pipe) {
        // Could not execute the command for some reason.
        return false;
    }

    try {
        while (fgets(line_buffer, sizeof(line_buffer), pipe) != nullptr) {
            sb << line_buffer;
        }

    } catch (...) {
        // Something went wrong while reading output from the command.
        // (likely out of memory)
        return false;
    }

    output = sb.str();
    return true;
}

/**
 * Parse the output lines from the addr2line tool.
 */
inline bool parse_a2l_output(const std::string& source_expr, std::string& source_path, int& line_number) {
    const auto source_parts = str::split(source_expr, ":");

    if (source_parts.size() < 2) {
        return false;
    }

    source_path = source_parts[0];
    std::stringstream ss;
    ss << std::dec << source_parts[1];
    ss >> line_number;
	return true;
}

/**
 * Get the name of the executable file.
 *
 * @returns The executable path, or empty string if it couldn't be determined.
 */
inline std::string get_executable_path() {
    char result[MOONLIGHT_PATH_MAX];

    ssize_t count = readlink("/proc/self/exe", result, MOONLIGHT_PATH_MAX);
    if (count > 0) {
        return std::string(result, count);

    } else {
        return "";
    }
}

inline fs::path shorten_path(const fs::path& path) {
    const auto cwd = fs::current_path();
    auto new_path = fs::weakly_canonical(path);
    if (new_path.string().starts_with(cwd.string())) {
        new_path = fs::path(".") / new_path.lexically_relative(cwd);
    }
    return new_path;
}

typedef void* BacktraceFrame;

/**
 * Retrieve the current stack frames as an array.
 */
inline std::vector<BacktraceFrame> backtrace_frames(int skip_frames = 2, int max_frames = 256) {
    std::vector<BacktraceFrame> frames(max_frames);
    int total_frames = backtrace(&frames[0], max_frames);
    frames.resize(total_frames);
    if (frames.size() >= skip_frames) {
        frames.erase(frames.begin(), frames.begin() + skip_frames);
    }
    return frames;
}

/**
 * Attempt to demangle the given mangled name.
 *
 * @returns The demangled name, or the unmodified name if demangling failed.
 */
inline std::string demangle(const std::string& name) {
    char* fname_buffer = nullptr;

    core::Finalizer finally([&]() {
        if (fname_buffer != nullptr) {
            free(fname_buffer);
        }
    });

    int status;
    fname_buffer = abi::__cxa_demangle(fname_buffer, nullptr, nullptr, &status);

    if (status == 0) {
        return std::string(fname_buffer);
    }

    return name;
}

// Forward declaration.
class Source;

/**
 * Represents a parsed, potentially demangled backtrace symbol.
 */
class BacktraceSymbol {
public:
    friend class Source;

    BacktraceSymbol(BacktraceFrame frame,
                    const std::string& raw,
                    const fs::path& module_path,
                    const std::string& function_name,
                    int offset)
    : _frame(frame), _raw(raw), _module_path(module_path),
    _function_name(function_name), _offset(offset) { }

    static BacktraceSymbol parse_raw_symbol(BacktraceFrame frame, const std::string& raw) {
        std::string module_path;
        std::string function_name;
        std::string offset_hex;
        int offset = 0;

        enum State {
            MODULE_NAME,
            FUNCTION_NAME,
            OFFSET_HEX,
            END
        };

        for (size_t x = 0, state = MODULE_NAME; state != END && x < raw.size(); x++) {
            int c = raw[x];

            switch (state) {
            case MODULE_NAME:
                if (c == '(') {
                    state = FUNCTION_NAME;
                } else {
                    module_path.push_back(c);
                }
                break;

            case FUNCTION_NAME:
                if (c == '+') {
                    state = OFFSET_HEX;

                } else {
                    function_name.push_back(c);
                }

                break;

            case OFFSET_HEX:
                if (c == ')') {
                    state = END;
                } else {
                    offset_hex.push_back(c);
                }

                break;

            case END:
                break;
            }
        }

        if (function_name.size() > 0) {
            function_name = demangle(function_name) + "()";
        }

        if (offset_hex.size() > 0) {
            std::stringstream ss;
            ss << std::hex << offset_hex;
            ss >> offset;
        }

        return BacktraceSymbol(frame, raw, module_path, function_name, offset);
    }

    static std::vector<BacktraceSymbol> parse_frames(const std::vector<BacktraceFrame>& frames) {
        std::vector<BacktraceSymbol> symbols;
        char** symbols_buffer = backtrace_symbols(&frames[0], frames.size());

        for (size_t x = 0; x < frames.size(); x++) {
            symbols.push_back(BacktraceSymbol::parse_raw_symbol(frames[x], symbols_buffer[x]));
        }

        free(symbols_buffer);
        return symbols;
    }

    BacktraceFrame frame() const {
        return _frame;
    }

    const std::string& raw() const {
        return _raw;
    }

    std::string module_file() const {
        return shorten_path(_module_path);
    }

    const fs::path& module_path() const {
        return _module_path;
    }

    const std::string& function_name() const {
        return _function_name;
    }

    size_t offset() const {
        return _offset;
    }

    std::string offset_hex() const {
        std::ostringstream sb;
        sb << std::hex << offset();
        return sb.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const BacktraceSymbol& symbol) {
        out << symbol.function_name() << " (" << symbol.module_file() << "+" << symbol.offset_hex() << ")";
        return out;
    }

private:
    BacktraceSymbol& function_name(const std::string& name) {
        _function_name = name;
        return *this;
    }

    BacktraceFrame _frame;
    std::string _raw;
    fs::path _module_path;
    std::string _function_name;
    size_t _offset;
};

class Source {
public:
    Source() : Source("", "", -1) { }

    Source(const fs::path& path, const std::string& function_name, int line_number)
    : _path(path), _function_name(function_name), _line_number(line_number) { }

    static std::vector<Source> from_backtrace_symbols(
        std::vector<BacktraceSymbol>& symbols, size_t batch_size = 100) {

        std::vector<Source> sources;

        std::string exe_name = get_executable_path();

        if (exe_name.size() == 0) {
            sources.insert(sources.end(), symbols.size(), {});
            return sources;
        }

        for (size_t x = 0; x < symbols.size(); x += batch_size) {
            std::ostringstream sb;
            sb << "addr2line -e " << exe_name << " -f -C";

            for (size_t y = x; y < x + batch_size && y < symbols.size(); y++) {
                sb << " " << symbols[y].offset_hex();
            }

            std::string output;
            int returncode;

            if (check(sb.str(), output, returncode) && returncode == 0) {
                auto lines = str::split(output, "\n");
                for (size_t z = 0; z + 1 < lines.size(); z += 2) {
                    std::string function_name = lines[z];
                    std::string source_path;
                    int line_number;

					if (function_name != "??" &&
                        parse_a2l_output(lines[z + 1], source_path, line_number)) {

                        if (! function_name.ends_with(")")) {
                            function_name += "()";
                        }

                        if (source_path != "??") {
                            sources.push_back(Source(source_path, function_name, line_number));

                        } else {
                            symbols[z / 2].function_name(function_name);
                            sources.push_back({});
                        }

                    } else {
                        sources.push_back({});
                    }
                }

            } else {
                sources.insert(sources.end(), std::min(batch_size, symbols.size() - x), {});
            }
        }

        return sources;
    }

    std::string file() const {
        return shorten_path(_path);
    }

    const fs::path& path() const {
        return _path;
    }

    const std::string& function_name() const {
        return _function_name;
    }

    int line_number() const {
        return _line_number;
    }

    bool is_nowhere() const {
        return _line_number <= 0;
    }

    friend std::ostream& operator<<(std::ostream& out, const Source& source) {
        if (source.is_nowhere()) {
            out << "(nowhere)";
        } else {
            out << source.function_name() << " (" << source.file() << ":" << source.line_number() << ")";
        }
        return out;
    }

private:
    fs::path _path;
    std::string _function_name;
    int _line_number;
};

class StackInfo {
public:
    StackInfo(const BacktraceSymbol& symbol, const Source source)
    : _symbol(symbol), _source(source) {  }

    static std::vector<StackInfo> parse_symbols(std::vector<BacktraceSymbol>& symbols) {
        return collect::zip<StackInfo>(symbols, Source::from_backtrace_symbols(symbols));
    }

    const BacktraceSymbol& symbol() const {
        return _symbol;
    }

    const Source& source() const {
        return _source;
    }

private:
    BacktraceSymbol _symbol;
    Source _source;
};

class StackTrace {
public:
    StackTrace() { }
    StackTrace(const Source& where)
    : _where(where) { }
    StackTrace(const std::vector<StackInfo>& stack_infos, Source where = {})
    : _where(where), _stack_infos(stack_infos) {
        chop_to_where();
    }

    static StackTrace generate(Source where = {}, size_t skip_frames = 2) {
        auto frames = backtrace_frames(skip_frames);
        auto symbols = BacktraceSymbol::parse_frames(frames);
        auto stack_infos = StackInfo::parse_symbols(symbols);
        return StackTrace(stack_infos, where);
    }

    const std::vector<StackInfo>& stack() const {
        return _stack_infos;
    }

    const Source& where() const {
        return _where;
    }

    bool contains_where() const {
        return find_where() != _stack_infos.end();
    }

    const void format(std::ostream& out, const std::string& prefix = "") const {
        auto joiner = "\n" + prefix;

        for (auto iter = stack().begin(); iter != stack().end(); iter++) {
            const auto& info = *iter;

            if (iter != stack().begin()) {
                out << joiner;
            } else {
                out << prefix;
            }

            if (! info.source().is_nowhere()) {
                out << info.source();
            } else {
                out << info.symbol();
            }
        }
    }

    friend std::ostream& operator<<(std::ostream& out, const StackTrace& stack) {
        stack.format(out);
        return out;
    }

private:
    std::vector<StackInfo>::const_iterator find_where() const {
        auto iter = _stack_infos.begin();

        if (_where.is_nowhere()) {
            return _stack_infos.end();
        }

        for (; iter != _stack_infos.end(); iter++) {
            if (iter->source().file() == _where.file() &&
                iter->source().line_number() == _where.line_number()) {
                break;
            }
        }

        return iter;
    }

    void chop_to_where() {
        auto iter = find_where();

        if (iter != _stack_infos.end()) {
            _stack_infos.erase(_stack_infos.begin(), iter);
        }
    }

    Source _where;
    std::vector<StackInfo> _stack_infos;
};

}

}

#endif /* !__MOONLIGHT_DEBUG_H */
