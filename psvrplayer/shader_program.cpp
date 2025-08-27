#include "shader_program.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// clang-format off
// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// clang-format on


bool CreateShaderProgram(unsigned int& program, unsigned char* vert_data,
    unsigned int vert_len, unsigned char* frag_data, unsigned int frag_len) {
  GLint success;
  std::string vs(vert_data, vert_data + vert_len);
  GLuint vs_i = glCreateShader(GL_VERTEX_SHADER);
  const GLchar* vs_char = vs.data();
  glShaderSource(vs_i, 1, &vs_char, NULL);
  glCompileShader(vs_i);
  glGetShaderiv(vs_i, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "ERROR: compilation of vertex shader failed" << std::endl;
    GLint log_len;
    glGetShaderiv(vs_i, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len) {
      std::vector<GLchar> msg(log_len);
      glGetShaderInfoLog(vs_i, log_len, NULL, msg.data());
      std::cerr << msg.data() << std::endl << std::endl;
    }
    return false;
  }

  std::string fs(frag_data, frag_data + frag_len);
  GLuint fs_i = glCreateShader(GL_FRAGMENT_SHADER);
  const GLchar* fs_char = fs.data();
  glShaderSource(fs_i, 1, &fs_char, NULL);
  glCompileShader(fs_i);
  glGetShaderiv(fs_i, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "ERROR: compilation of fragment shader failed" << std::endl;
    GLint log_len;
    glGetShaderiv(fs_i, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len) {
      std::vector<GLchar> msg(log_len);
      glGetShaderInfoLog(fs_i, log_len, NULL, msg.data());
      std::cerr << msg.data() << std::endl << std::endl;
    }
    return false;
  }

  GLuint ps_i = glCreateProgram();

  glAttachShader(ps_i, vs_i);
  glAttachShader(ps_i, fs_i);
  glLinkProgram(ps_i);
  glGetProgramiv(ps_i, GL_LINK_STATUS, &success);
  if (!success) {
    std::cerr << "ERROR: compilation of shader program failed" << std::endl;
    GLint log_len;
    glGetProgramiv(ps_i, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len) {
      std::vector<GLchar> msg(log_len);
      glGetProgramInfoLog(ps_i, log_len, NULL, msg.data());
      std::cerr << msg.data() << std::endl << std::endl;
    }
  }

  glDeleteShader(fs_i);
  glDeleteShader(vs_i);

  program = ps_i;
  return true;
}

void DeleteShaderProgram(unsigned int program) { glDeleteProgram(program); }
