/*
 * moonlight/opengl.h: OpenGL 3.0+ specializations and tools.
 *
 * Author: Lain Musgrove (lainproliant)
 * Date:   Friday Jun 21 2019
 */
#pragma once
#include "moonlight/core.h"
#include <GL/glew.h>
#include <string>
#include <memory>
#include <vector>
#include <tinyformat/tinyformat.h>

namespace moonlight {
namespace opengl {

//-------------------------------------------------------------------
class Error : public core::Exception {
   using Exception::Exception;
};

//-------------------------------------------------------------------
class Shader {
public:
   Shader(const std::string& name, const std::string& code, GLenum shader_type) {
      GLint result = GL_FALSE;
      int log_length;

      id_ = glCreateShader(shader_type);
      const char* code_ptr = code.c_str();
      glShaderSource(id(), 1, &code_ptr, nullptr);
      glCompileShader(id());

      glGetShaderiv(id(), GL_COMPILE_STATUS, &result);

      if (result == GL_FALSE) {
         glGetShaderiv(id(), GL_INFO_LOG_LENGTH, &log_length);
         std::vector<char> error_msg(log_length + 1);
         glGetShaderInfoLog(id(), log_length, nullptr, &error_msg[0]);
         throw Error(tfm::format("Failed to compile shader (%s): %s",
                                 name, &error_msg[0]));
      }

   }

   ~Shader() {
      glDeleteShader(id());
   }

   static std::shared_ptr<Shader> load(const std::string& filename,
                                       GLenum shader_type) {
      auto infile = moonlight::file::open_r(filename);
      return std::make_shared<Shader>(filename,
                                      moonlight::file::to_string(infile),
                                      shader_type);
   }

   GLuint id() const {
      return id_;
   }

private:
   GLuint id_;
};

//-------------------------------------------------------------------
class Program {
public:
   Program(const std::vector<std::shared_ptr<Shader>>& shaders,
           const std::vector<std::pair<GLuint, std::string>>& attribs = {}) {
      GLint result = GL_FALSE;
      int log_length;

      id_ = glCreateProgram();

      for (auto& shader : shaders) {
         glAttachShader(id(), shader->id());
      }

      for (auto& attrib : attribs) {
         glBindAttribLocation(id(), attrib.first, attrib.second.c_str());
      }

      glLinkProgram(id());

      glGetProgramiv(id(), GL_LINK_STATUS, &result);

      if (result == GL_FALSE) {
         glGetProgramiv(id(), GL_INFO_LOG_LENGTH, &log_length);
         std::vector<char> error_msg(log_length + 1);
         glGetProgramInfoLog(id(), log_length, nullptr, &error_msg[0]);
         throw Error(tfm::format("Failed to link shaders: %s",
                                 &error_msg[0]));
      }

      for (auto& shader : shaders) {
         glDetachShader(id(), shader->id());
      }
   }

   ~Program() {
      glDeleteProgram(id());
   }

   static std::shared_ptr<Program> link(
       const std::vector<std::shared_ptr<Shader>>& shaders,
       const std::vector<std::pair<GLuint, std::string>>& attribs = {}) {
      return std::make_shared<Program>(shaders, attribs);
   }
   
   GLuint id() const {
      return id_;
   }

private:
   GLuint id_;
};

}
}
