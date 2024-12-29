/*
 * ## moonlight/cli.h: Useful tools for building command line apps. -
 *
 * Author: Lain Supe (lainproliant)
 * Date: Thursday, Dec 21 2017
 *
 * ## Usage ---------------------------------------------------------
 * This library provides a simplified alternative to `getopt(3)` in the form of
 * `cli::parse`.  This function accepts command line arguments, a set of flag
 * names, and a set of option names, and returns a `cli::CommandLine` object
 * which can be used to query the result of the parsing of those flags and
 * options from the given command line arguments.  Arguments can be provided
 * either as a vector of `std::string` or the `int argc, char** argv` pair that
 * is passed to `main()`.
 *
 * In this example, we are parsing a required "input" option, an optional
 * "output" option, and a "verbose" flag.  Note that single-character arguments
 * can be provided in the form `-x` or `--x`, and that multi-character arguments
 * must be provided in the form `--xyz`, as the argument `-xyz` is interpreted
 * as specifying "x", "y", and "z" single-character flags.
 *
 * ```
 * auto cli = cli::parse(argc, argv,
 *                       {"i", "input", "o", "output"},
 *                       {"v", "verbose"});
 * bool verbose = cli.check("v", "verbose");
 * std::string input = cli.require("i", "input");
 * std::optional<std::string> output = cli.get("o", "output");
 * ```
 *
 * The first parameter in the argument list is assumed to be the program name,
 * and is available via `get_program_name()` on the `cli` object.
 *
 * ## Other Functions -----------------------------------------------
 * `cli.h` also provides two other useful utility functions:
 *
 * - `cli::argv_to_vector(argc, argv)`: Converts `argc` and `argb` from `main()`
 *   into a `std::vector<std::string>`.
 * - `cli::getenv(name)`: Gets the value of the given environment variable if
 *   it exists as an optional, otherwise returns an empty optional.
 */
#ifndef __MOONLIGHT_CLI_H
#define __MOONLIGHT_CLI_H

#include <map>
#include <vector>
#include <set>
#include <string>

#include "moonlight/exceptions.h"
#include "moonlight/collect.h"
#include "moonlight/variadic.h"
#include "moonlight/slice.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace moonlight {
namespace cli {

/**
 * Convert argc and argv from main() into a vector of strings.
 */
inline std::vector<std::string> argv_to_vector(int argc, char** argv) {
    std::vector<std::string> vec;

    for (int x = 0; x < argc; x++) {
        vec.push_back(std::string(argv[x]));
    }

    return vec;
}

/**
*/
inline std::optional<std::string> getenv(const std::string& name) {
    const char* value = std::getenv(name.c_str());
    if (value != nullptr) {
        return value;
    } else {
        return {};
    }
}

/**
*/
class CommandLine {
 public:
     CommandLine() { }
     virtual ~CommandLine() { }

     const std::string& get_program_name() const {
         return program_name;
     }

     bool _check(const std::string& flag) const {
         return flags.contains(flag);
     }

     template<typename F, typename... FV>
     bool check(const F& flag, const FV&... flags) const {
         if (_check(flag)) {
             return true;
         } else {
             return check(flags...);
         }
     }

     bool check() const {
         return false;
     }

     std::optional<const std::string> _get(const std::string& opt) const {
         if (opts.contains(opt)) {
             return opts.at(opt);
         } else {
             return {};
         }
     }

     std::optional<const std::string> get() const {
         return {};
     }

     template<typename O, typename... OD>
     std::optional<const std::string> get(const O& opt, const OD&... opts) const {
         auto value = _get(opt);
         if (value) {
             return *value;
         } else {
             return get(opts...);
         }
     }

     template<typename... O>
     std::string require(const O&... opts) {
         auto value = get(opts...);
         if (value) {
             return *value;
         } else {
             THROW(core::UsageError, str::cat(
                     std::string("Missing required option "),
                     str::join(variadic::to_vector(std::string(opts)...), "/")));
         }
     }

     const std::vector<std::string>& args() const {
         return args_;
     }

     static CommandLine parse(const std::vector<std::string>& argv,
                              const std::set<std::string>& flag_names = {},
                              const std::set<std::string>& opt_names = {}) {
         CommandLine results;
         results.program_name = argv[0];

         for (unsigned int x = 1; x < argv.size(); x++) {
             auto& arg = argv[x];
             if (arg == "--") {
                 // All following args are true args.
                 for (x++; x < argv.size(); x++) {
                     results.args_.push_back(argv[x]);
                 }

             } else if (str::startswith(arg, "--")) {
                 // This is a longopt.
                 auto longopt = slice(arg, 2, {});

                 if (collect::contains(longopt, '=')) {
                     // This is a longopt option with a value in the same arg.
                     auto parts = str::split(longopt, "=");
                     auto opt = parts[0];
                     auto value = str::join(slice(parts, 1, {}), "=");

                     if (collect::contains(opt_names, opt)) {
                         results.opts[opt] = value;
                         results.multi_opts.insert(std::pair<std::string, std::string>(opt, value));

                     } else {
                         THROW(core::UsageError, str::cat(
                                 "Unknown option '", opt, "'."));
                     }

                 } else if (collect::contains(flag_names, longopt)) {
                     // This is a longopt flag.
                     results.flags.insert(longopt);

                 } else if (collect::contains(opt_names, longopt)) {
                     // This is a longopt option, the next arg is the opt value.
                     if (x + 1 < argv.size()) { auto value = argv[++x];
                         results.opts[longopt] = value;
                         results.multi_opts.insert(std::pair<std::string, std::string>(longopt, value));

                     } else {
                         THROW(core::UsageError, str::cat(
                                 "Missing required value for option '--", longopt, "'."));
                     }

                 } else {
                     THROW(core::UsageError, str::cat(
                             "Unknown flag or option '--", longopt, "'."));
                 }

             } else if (str::startswith(arg, "-") && arg.size() > 1) {
                 // This is one or more shortopts.
                 auto shortopts = slice(arg, 1, {});
                 for (unsigned int y = 0; y < shortopts.size(); y++) {
                     auto shortopt = str::chr(shortopts[y]);
                     if (collect::contains(flag_names, shortopt)) {
                         results.flags.insert(shortopt);

                     } else if (collect::contains(opt_names, shortopt)) {
                         if (y + 1 < shortopts.size()) {
                             // The rest of shortopts is the opt value.
                             results.opts.insert(
                                 std::pair<std::string, std::string>{shortopt, slice(shortopts, y + 1, {})});
                             y = shortopts.size();

                         } else if (x + 1 < argv.size()) {
                             // The next arg is the opt value.
                             results.opts.insert(std::pair<std::string, std::string>{shortopt, argv[++x]});

                         } else {
                             THROW(core::UsageError, str::cat(
                                     "Missing required parameter for '-", shortopt, "'."));
                         }
                     } else {
                         THROW(core::UsageError, str::cat(
                                 "Unknown flag or option '-", shortopt, "'."));
                     }
                 }

             } else {
                 // This is just a plain old arg.
                 results.args_.push_back(arg);
             }
         }

         return results;
     }

 private:
     std::string program_name;
     std::set<std::string> flags;
     std::map<std::string, int> flag_counts;
     std::map<std::string, std::string> opts;
     std::multimap<std::string, std::string> multi_opts;
     std::vector<std::string> args_;
};

/**
*/
inline CommandLine parse(const std::vector<std::string>& argv,
                         const std::set<std::string>& flag_names = {},
                         const std::set<std::string>& opt_names = {}) {
    return CommandLine::parse(argv, flag_names, opt_names);
}

/**
*/
inline CommandLine parse(int argc_in, char** argv_in,
                         const std::set<std::string>& flag_names = {},
                         const std::set<std::string>& opt_names = {}) {
    return parse(argv_to_vector(argc_in, argv_in), flag_names, opt_names);
}

}  // namespace cli
}  // namespace moonlight

#endif /* __MOONLIGHT_CLI_H */
