#include "transformer.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>


// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "play_screen.h"


// TODO Bad style
GLchar kVertexShader[] = "#version 330 core \n\
\n\
layout (location = 0) in vec3 position; \n\
out vec2 TexCoord; \n\
\n\
void main() \n\
{ \n\
    gl_Position = vec4(position.x, position.y, position.z, 1.0); \n\
    TexCoord = position.xy / 2 + vec2(0.5f, 0.5f); \n\
} \n\
";

// TODO Bad style
GLchar kFragmentShader[] = "#version 330 core \n\
\n\
out vec4 color; \n\
in vec2 TexCoord; \n\
uniform sampler2D ourTexture; \n\
\n\
void main() \n\
{ \n\
  if (TexCoord.x < 0.1 || TexCoord.x > 0.9f || TexCoord.y < 0.1f || TexCoord.y > 0.9f) \n\
  { color = vec4(1.0f, 0, 0, 1.0f); return; } \n\
//  color = vec4(TexCoord.y, TexCoord.x, 1.0f, 1.0f); \n\
//  return; \n\
  vec4 tc = texture(ourTexture, TexCoord); \n\
  color = tc; \n\
  return; \n\
  color = color / 2 + tc; \n\
  color = tc; \n\
//  color = vec4(1.0f, 0.5f, 0.2f, 0.5f); \n\
} \n\
";

class GlProgramm: public Transformer {
 public:
  GlProgramm(IPlayScreenPtr screen);
  ~GlProgramm();

  void SetImage(int width, int height, int align_width, const void* data) override;
 private:
  GlProgramm() = delete;
  GlProgramm(const GlProgramm&) = delete;
  GlProgramm(GlProgramm&&) = delete;
  GlProgramm& operator=(const GlProgramm&) = delete;
  GlProgramm& operator=(GlProgramm&&) = delete;

  std::thread transform_thread_;
  IPlayScreenPtr screen_;
  std::vector<char> buffer_;

  std::condition_variable update_var_;
  bool update_flag_;
  bool shutdown_flag_;
  std::mutex update_lock_;

  int align_width_;
  int height_;

  void Processing();
};




Transformer* CreateTransformer(IPlayScreenPtr screen) {
  try {
    return new GlProgramm(screen);
  }  catch (...) {

  }
  return nullptr;
}

GlProgramm::GlProgramm(IPlayScreenPtr screen) {
  screen_ = screen;
  shutdown_flag_ = false;
  update_flag_ = false;
  if (!screen_) {
    std::cerr << "ERROR: Can't got screen" << std::endl;
    throw std::logic_error(__FUNCTION__);
  }

  std::thread t([this](){ Processing(); });
  std::swap(transform_thread_, t);
  assert(!t.joinable());
}

GlProgramm::~GlProgramm() {
  std::unique_lock<std::mutex> lk(update_lock_);
  shutdown_flag_ = true;
  update_var_.notify_all();
  lk.unlock();
  if (transform_thread_.joinable()) {
    transform_thread_.join();
  }
}

void GlProgramm::SetImage(int width, int height, int align_width, const void* data) {
  const size_t kPixelSize = 4;
  size_t data_size = align_width * height * kPixelSize;
  try {
    // TODO Check the buffer is free now before resize
    buffer_.resize(data_size);
    std::memcpy(buffer_.data(), data, data_size);

    align_width_ = align_width;
    height_ = height;

    std::unique_lock<std::mutex> lk(update_lock_);
    update_flag_ = true;
    update_var_.notify_all();
    lk.unlock();
  } catch (std::bad_alloc&) {
  }
}

void GlProgramm::Processing() {
  screen_->MakeScreenCurrent();
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Can't initialize Glad context. Maybe a logic error: make current context for window first" << std::endl;
    throw std::runtime_error("Can't initialize Glad");
  }


  GLfloat vertices[] = {
      -1.0f, -1.0f, 0.0f,
       1.0f, -1.0f, 0.0f,
       0.0f,  1.0f, 0.0f
  };

  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  GLuint vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const GLchar* v = kVertexShader;
  glShaderSource(vertexShader, 1, &v, NULL);
  glCompileShader(vertexShader);

  GLint success;
  GLchar infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  GLuint fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  const GLchar* f = kFragmentShader;
  glShaderSource(fragmentShader, 1, &f, NULL);
  glCompileShader(fragmentShader);
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  GLuint shaderProgram;
  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::PROGRAMM::COMPILATION_FAILED\n" << infoLog << std::endl;
  }

  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  GLuint tx;
  glGenTextures(1, &tx);
  glBindTexture(GL_TEXTURE_2D, tx);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  while (true) {
    std::unique_lock<std::mutex> lk(update_lock_);
    update_var_.wait(lk, [this](){ return update_flag_ || shutdown_flag_; });
    if (shutdown_flag_) {
      break;
    }
    assert(update_flag_);
    update_flag_ = false;

    static float color = 0.0f;
    color += 1.0f / 250.0;
    if (color > 1.0) {
      color = 0.0f;
    }
    glClearColor(color, color, color, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, tx);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, align_width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer_.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    glViewport(0, 0, 1900, 1000);
    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, tx);
    glBindVertexArray(vertex_array);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    screen_->DisplayBuffer();
  }
}
