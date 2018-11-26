/*
 * moonlight/lex.h: Simple general-purpose state-based lexer.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date: Monday, Oct 15 2018
 */
#pragma once

#include <regex>
#include "moonlight/core.h"
#include "moonlight/automata.h"
#include "moonlight/json.h"

namespace moonlight {
namespace lex {

const std::string IGNORE = "@ignore";
const std::string PUSH = "@push";
const std::string POP = "@pop";
const std::string ROOT = "@root";

//-------------------------------------------------------------------
class ContentView {
public:
   ContentView(const std::string& filename,
               int line, int col,
               const std::string& text) :
   filename_(filename), line_(line), col_(col), text_(text) { }

   const std::string& filename() const { return filename_; }
   int line() const { return line_; }
   int col() const { return col_; }
   const std::string& text() const { return text_; }

private:
   std::string filename_;
   int line_;
   int col_;
   std::string text_;
};

//-------------------------------------------------------------------
class MatchResult {
public:
   MatchResult(const ContentView& content,
               const std::vector<std::string>& submatches) :
   content_(content), submatches_(submatches) { }

   static MatchResult from_smatch(const ContentView& src, const std::smatch& sm) {
      ContentView result_content(src.filename(), src.line(),
                                 src.col() + sm.position(), sm[0]);
      std::vector<std::string> submatches;
      std::copy(sm.begin() + 1, sm.end(), submatches.begin());
      return MatchResult(result_content, submatches);
   }

   const ContentView& content() const { return content_; }
   const std::string& sub(unsigned int n) const {
      if (n >= submatches_.size()) {
         throw core::IndexException("Invalid submatch index.");
      }
      return submatches_.at(n);
   }

private:
   ContentView content_;
   std::vector<std::string> submatches_;
};

//-------------------------------------------------------------------
class Error : public core::Exception {
public:
   Error(const std::string& message,
         std::optional<std::string> state = {},
         std::optional<ContentView> content = {}) :
   Exception(construct_message(message, state, content)) { }

private:
   static std::string construct_message(const std::string& message,
                                        std::optional<std::string> state,
                                        std::optional<ContentView> content) {
      if (state && content) {
         return str::cat(
             message, " (in ", content->filename(), " @ ",
             *state, ":", content->line(), ":", content->col(), ")");
      } else {
         return message;
      }
   }
};

//-------------------------------------------------------------------
class LexemeType {
public:
   LexemeType(const std::string& name,
              const std::string& next_state,
              std::vector<std::regex> patterns) :
   name_(name), next_state_(next_state), patterns_(patterns) { }

   LexemeType(const std::string& name,
              const std::string& next_state,
              const std::regex& pattern) :
   name_(name), next_state_(next_state), patterns_({pattern}) { }

   std::optional<MatchResult> match(const ContentView& content) {
      std::smatch sm;

      for (auto rx : patterns_) {
         std::regex_match(content.text(), sm, rx);
         if (sm.size() > 0) {
            return MatchResult::from_smatch(content, sm);
         }
      }

      return {};
   }

private:
   std::string name_;
   std::string next_state_;
   std::vector<std::regex> patterns_;
};

//-------------------------------------------------------------------
class Lexeme {
public:
   Lexeme(const LexemeType& type, const MatchResult& match) :
   type_(type), match_(match) { }

   const LexemeType& type() const {
      return type_;
   }

   const MatchResult& match() const {
      return match_;
   }

private:
   LexemeType type_;
   MatchResult match_;
};

}
}
