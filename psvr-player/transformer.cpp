#include "transformer.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>


// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "play_screen.h"
#include "shaders/output.h"


std::string UseConstOrLoadTextFile(const char* const_var, const char* fname) {
  if (const_var[0]) {
    // Константа не пустая
    return const_var;
  }

  std::stringstream str;
  str << "../psvr-player/shaders/" << fname;
  std::ifstream f(str.str());
  std::string line;
  std::stringstream res;
  while (f) {
    std::getline(f, line);
    res << line << std::endl;
  }

  return res.str();
}


// Макрос для получения кода шейдеров как текст
// Предварительно должна существовать текстовая константа с префиксом k,
// т.е. если передаётся переменная VariableOne, то должна быть константа kVariableOne
// Если константа не пустая, то код шейдера берётся из константы
// Если константа пустая, то код берётся из файла
// Параметры:
// variable - имя переменной, которая будет создана как указатель на код
// name - имя файла, который грузится, если константа пустая
#define SHADER(variable, fname) \
auto variable##Code = UseConstOrLoadTextFile(k##variable, fname); \
const GLchar* variable = variable##Code.c_str();


GLfloat OutputSceneVertices[] = {
  -1.0f, -1.0f, 0.0f,
  -1.0f,  1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
  -1.0f,  1.0f, 0.0f,
   1.0f, -1.0f, 0.0f,
   1.0f,  1.0f, 0.0f
};

const GLint OutputSceneVerticesAmount = 6;



class GlProgramm: public Transformer {
 public:
  GlProgramm(IPlayScreenPtr screen);
  ~GlProgramm();

  void SetImage(Frame&& frame) override;
 private:
  GlProgramm() = delete;
  GlProgramm(const GlProgramm&) = delete;
  GlProgramm(GlProgramm&&) = delete;
  GlProgramm& operator=(const GlProgramm&) = delete;
  GlProgramm& operator=(GlProgramm&&) = delete;

  std::thread transform_thread_;
  IPlayScreenPtr screen_;

  std::vector<Frame> last_frames_;
  std::mutex last_frames_lock_;

  // Переменная обновления работает в два флага: shutdown_flag_ и не пустой last_frames_
  // last_frames_ не под блокировкой переменной (update_lock_), поэтому нужно проверять
  // каждый раз
  std::condition_variable update_var_;
  bool shutdown_flag_;
  std::mutex update_lock_;

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

void GlProgramm::SetImage(Frame&& frame) {
  std::unique_lock<std::mutex> fr_lock(last_frames_lock_);
  last_frames_.push_back(std::move(frame));
  fr_lock.unlock();
  update_var_.notify_all();
}

void GlProgramm::Processing() {
  screen_->MakeScreenCurrent();
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Can't initialize Glad context. Maybe a logic error: make current context for window first" << std::endl;
    throw std::runtime_error("Can't initialize Glad");
  }



  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(OutputSceneVertices),
      OutputSceneVertices, GL_STATIC_DRAW);

  GLuint vertexShader;
  SHADER(OutputVertexShader, "output.vert");
  vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &OutputVertexShader, NULL);
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
  SHADER(OutputFragmentShader, "output.frag");
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &OutputFragmentShader, NULL);
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
    if (shutdown_flag_) { break; }
    update_var_.wait(lk);
    if (shutdown_flag_) { break; }
    lk.unlock();

    // Вытащим все пришедшие кадры, их может и не быть (ложное слетание с wait)
    std::vector<Frame> last;
    std::unique_lock<std::mutex> fl(last_frames_lock_);
    std::swap(last, last_frames_);
    fl.unlock();

    if (last.empty()) {
      continue;
    }

    // Выбираем последний кадр в работу. Остальные возвращаем в пул
    Frame frame = std::move(last.back());
    last.pop_back();
    while (!last.empty()) {
      ReleaseFrame(std::move(last.back()));
      last.pop_back();
    }

    glBindTexture(GL_TEXTURE_2D, tx);
    int width, height, align_width;
    frame.GetSizes(&width, &height, &align_width, nullptr);
    size_t data_size;
    void* data = frame.GetData(data_size);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, align_width, height, 0, GL_RGBA,
        GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    ReleaseFrame(std::move(frame));

    int scrw, scrh;
    screen_->GetFrameSize(scrw, scrh);
    glViewport(0, 0, scrw, scrh);

    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, tx);
    glBindVertexArray(vertex_array);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, OutputSceneVerticesAmount);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    screen_->DisplayBuffer();
  }
}
