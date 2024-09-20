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

#include "frame_buffer.h"
#include "play_screen.h"
#include "shader_program.h"

GLfloat OutputSceneVertices[] = {-1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
    -1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f};

const GLint OutputSceneVerticesAmount = 6;

/*! Массив вершин для отрисовки, зарегистрированный как объект в opengl */
struct VertexArray {
  unsigned int array_id;  //!< Идентификатор массива вершин
  unsigned int array_size;  //!< Размер массива, количество вершин в отрисовку
};

class GlProgramm : public Transformer {
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

  // Переменная обновления работает в два флага: shutdown_flag_ и не пустой
  // last_frames_ last_frames_ не под блокировкой переменной (update_lock_),
  // поэтому нужно проверять каждый раз
  std::condition_variable update_var_;
  bool shutdown_flag_;
  std::mutex update_lock_;

  unsigned int split_program_;

  VertexArray cube_vertex_;  //!< Вершины для кубической сцены

  void Processing();

  void SplitScreen(unsigned int texture, FrameBuffer& left, FrameBuffer& right,
      unsigned int vertex_id, unsigned int vertex_amount);

  /*! Создать/зарегистрировать массив вершин для отрисовки куба */
  bool CreateCubeVertex(VertexArray& vertex);
};

Transformer* CreateTransformer(IPlayScreenPtr screen) {
  try {
    return new GlProgramm(screen);
  } catch (...) {
  }
  return nullptr;
}

GlProgramm::GlProgramm(IPlayScreenPtr screen) : split_program_(0) {
  screen_ = screen;
  shutdown_flag_ = false;
  if (!screen_) {
    std::cerr << "ERROR: Can't got screen" << std::endl;
    throw std::logic_error(__FUNCTION__);
  }

  std::thread t([this]() { Processing(); });
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
    std::cerr << "Can't initialize Glad context. Maybe a logic error: make "
                 "current context for window first"
              << std::endl;
    throw std::runtime_error("Can't initialize Glad");
  }

  FrameBuffer left_eye, right_eye;
  if (!CreateFrameBuffer(left_eye)) {
    throw std::runtime_error("Can't initialize left framebuffer");
  }
  if (!CreateFrameBuffer(right_eye)) {
    throw std::runtime_error("Can't initialize right framebuffer");
  }

  if (!CreateShaderProgram("split", split_program_)) {
    throw std::runtime_error("Can't create split program");
  }

  unsigned int output_program;
  if (!CreateShaderProgram("output", output_program)) {
    throw std::runtime_error("Can't create output program");
  }

  if (!CreateCubeVertex(cube_vertex_)) {
    throw std::runtime_error("Can't create cube scene");
  }

  // Буфер и др. для финального вывода в окно
  GLuint VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(OutputSceneVertices),
      OutputSceneVertices, GL_STATIC_DRAW);
  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);

  // Входная текстура из проигрывателя
  GLuint tx;
  glGenTextures(1, &tx);
  glBindTexture(GL_TEXTURE_2D, tx);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  while (true) {
    std::unique_lock<std::mutex> lk(update_lock_);
    if (shutdown_flag_) {
      break;
    }
    update_var_.wait(lk);
    if (shutdown_flag_) {
      break;
    }
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, align_width, height, 0, GL_BGRA,
        GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    ReleaseFrame(std::move(frame));

    SplitScreen(
        tx, left_eye, right_eye, vertex_array, OutputSceneVerticesAmount);

    int scrw, scrh;
    screen_->GetFrameSize(scrw, scrh);
    glViewport(0, 0, scrw, scrh);

    glUseProgram(output_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, left_eye.texture);
    GLint loc = glGetUniformLocation(output_program, "left_image");
    glUniform1i(loc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, right_eye.texture);
    loc = glGetUniformLocation(output_program, "right_image");
    glUniform1i(loc, 1);
    glActiveTexture(GL_TEXTURE0);

    //    glBindTexture(GL_TEXTURE_2D, left_eye.texture);
    glBindVertexArray(vertex_array);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, OutputSceneVerticesAmount);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    screen_->DisplayBuffer();
  }

  glDeleteTextures(1, &tx);

  DeleteFrameBuffer(left_eye);
  DeleteFrameBuffer(right_eye);
  DeleteShaderProgram(split_program_);
  DeleteShaderProgram(output_program);
}

void GlProgramm::SplitScreen(unsigned int texture, FrameBuffer& left,
    FrameBuffer& right, unsigned int vertex_id, unsigned int vertex_amount) {
  // Разделим текстуру на две
  GLint loc;

  // Левая
  glBindFramebuffer(GL_FRAMEBUFFER, left.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glUseProgram(split_program_);

  loc = glGetUniformLocation(split_program_, "param");
  glUniform1i(loc, 0);

  glBindTexture(GL_TEXTURE_2D, texture);
  glBindVertexArray(vertex_id);
  glDrawArrays(GL_TRIANGLES, 0, vertex_amount);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Правая
  glBindFramebuffer(GL_FRAMEBUFFER, right.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glUseProgram(split_program_);
  glBindTexture(GL_TEXTURE_2D, texture);

  loc = glGetUniformLocation(split_program_, "param");
  glUniform1i(loc, 1);

  glBindVertexArray(vertex_id);
  glDrawArrays(GL_TRIANGLES, 0, vertex_amount);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool GlProgramm::CreateCubeVertex(VertexArray& vertex) {
  const GLuint kVertexAmount = 36;
  GLfloat cube[kVertexAmount * 3] = {// Far face
      // x   y      z
      -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
      // Left face
      // x   y      z
      -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      // Right face
      // x   y      z
      1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
      // Far face
      // x   y      z
      -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f,
      1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      // Top face
      // x   y      z
      -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f,
      // Bottom face
      // x   y      z
      -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f};

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, kVertexAmount, cube, GL_STATIC_DRAW);
  GLuint vertex_array;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &vbo);
  vertex.array_id = vertex_array;
  vertex.array_size = kVertexAmount;
  return true;
}
