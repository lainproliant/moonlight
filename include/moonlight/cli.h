/*
 * moonlight/cli.h: Useful tools for building command line apps.
 *
 * Author: Lain Supe (lainproliant)
 * Date: Thursday, Dec 21 2017
 */
#ifndef __MOONLIGHT_CLI_H
#define __MOONLIGHT_CLI_H

#include "moonlight/exceptions.h"
#include "moonlight/collect.h"
#include "moonlight/variadic.h"
#include "moonlight/maps.h"
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
std::vector<std::string> argv_to_vector(int argc, char** argv) {
   std::vector<std::string> vec;

   for (int x = 0; x < argc; x++) {
      vec.push_back(std::string(argv[x]));
   }

   return vec;
}

//-------------------------------------------------------------------
std::optional<std::string> getenv(const std::string& name) {
   const char* value = std::getenv(name.c_str());
   if (value != nullptr) {
      return value;
   } else {
      return {};
   }
}

//-------------------------------------------------------------------
class CommandLineException : public core::Exception {
   using core::Exception::Exception;
};

//-------------------------------------------------------------------
class CommandLine {
public:
   CommandLine() { }
   virtual ~CommandLine() { }

   const std::string& get_program_name() const {
      return program_name;
   }

   bool _check(const std::string& flag) const {
      return collect::contains(flags, flag);
   }

   template<typename F, typename... FV>
   bool check(const F& flag, const FV&... flags) {
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
      if (collect::contains(opts, opt)) {
         return maps::const_value(opts, opt);
      } else {
         return {};
      }
   }

   std::optional<const std::string> get() const {
      return {};
   }

   template<typename O, typename... OD>
   std::optional<const std::string> get(const O& opt, const OD&... opts) {
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
         throw CommandLineException(str::cat(
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
                  results.multi_opts.insert({opt, value});

               } else {
                  throw CommandLineException(str::cat(
                     "Unknown option '", opt, "'."
                  ));
               }

            } else if (collect::contains(flag_names, longopt)) {
               // This is a longopt flag.
               results.flags.insert(longopt);

            } else if (collect::contains(opt_names, longopt)) {
               // This is a longopt option, the next arg is the opt value.
               if (x + 1 < argv.size()) {
                  auto value = argv[++x];
                  results.opts[longopt] = value;
                  results.multi_opts.insert({longopt, value});

               } else {
                  throw CommandLineException(str::cat(
                     "Missing required value for option '--", longopt, "'."
                  ));
               }

            } else {
               throw CommandLineException(str::cat(
                  "Unknown flag or option '--", longopt, "'."
               ));
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
                         {shortopt, slice(shortopts, y + 1, {})});
                     y = shortopts.size();

                  } else if (x + 1 < argv.size()) {
                     // The next arg is the opt value.
                     results.opts.insert({shortopt, argv[++x]});

                  } else {
                     throw CommandLineException(str::cat(
                        "Missing required parameter for '-", shortopt, "'."
                     ));
                  }
               } else {
                  throw CommandLineException(str::cat(
                     "Unknown flag or option '-", shortopt, "'."
                  ));
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

//-------------------------------------------------------------------
CommandLine parse(const std::vector<std::string>& argv,
                  const std::set<std::string>& flag_names = {},
                  const std::set<std::string>& opt_names = {}) {
   return CommandLine::parse(argv, flag_names, opt_names);
}

//-------------------------------------------------------------------
CommandLine parse(int argc_in, char** argv_in,
                  const std::set<std::string>& flag_names = {},
                  const std::set<std::string>& opt_names = {}) {
   return parse(argv_to_vector(argc_in, argv_in), flag_names, opt_names);
}
}
}

#endif /* __MOONLIGHT_CLI_H */
