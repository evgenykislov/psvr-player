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

// clang-format off
// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// clang-format on

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "frame_buffer.h"
#include "play_screen.h"
#include "shader_program.h"

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
  void SetEyeSwap(bool swap) override;
  void SetViewPoint(float x_disp, float y_disp) override;


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
  bool swap_eyes_setting_;  //!< Настройка по смене порядка изображений для
                            //!< глаз. Настройка под блокировкой update_lock_
  float x_angle_;
  float y_angle_;

  // Переменная обновления работает в два флага: shutdown_flag_ и не пустой
  // last_frames_ last_frames_ не под блокировкой переменной (update_lock_),
  // поэтому нужно проверять каждый раз
  std::condition_variable update_var_;
  bool shutdown_flag_;
  std::mutex update_lock_;

  // Переменные для работы только в функциях процессинга
  unsigned int split_program_;
  unsigned int half_cilinder_program_;
  glm::mat4 projection_matrix_;  //!< Проекционная матрица
  VertexArray cube_vertex_;  //!< Вершины для кубической сцены (формирование
                             //!< полусфер и т.д.)
  VertexArray flat_vertex_;  //!< Вершины для плоской сцены (вывод изображений)

  void Processing();

  // TODO ???
  void SplitScreen(unsigned int texture, FrameBuffer& left, FrameBuffer& right,
      unsigned int width, unsigned int aligned_width);

  void HalfCilinder(const FrameBuffer& in_buffer, const FrameBuffer& out_buffer,
      const glm::mat4& transform);

  /*! Удалить массив вершин */
  void DeleteVertex(VertexArray& vertex);

  /*! Создать/зарегистрировать массив вершин для отрисовки куба */
  bool CreateCubeVertex(VertexArray& vertex);

  // TODO ???
  bool CreateFlatVertex(VertexArray& vertex);
};

Transformer* CreateTransformer(IPlayScreenPtr screen) {
  try {
    return new GlProgramm(screen);
  } catch (...) {
  }
  return nullptr;
}

GlProgramm::GlProgramm(IPlayScreenPtr screen)
    : split_program_(0), half_cilinder_program_(0), swap_eyes_setting_(false) {
  screen_ = screen;
  shutdown_flag_ = false;
  cube_vertex_.array_id = 0;
  flat_vertex_.array_id = 0;

  if (!screen_) {
    std::cerr << "ERROR: Can't got screen" << std::endl;
    throw std::logic_error(__FUNCTION__);
  }

  std::thread t([this]() { Processing(); });
  std::swap(transform_thread_, t);
  assert(!t.joinable());
}


// TODO Отладка
static float minx = 0;
static float maxx = 0;


GlProgramm::~GlProgramm() {
  std::unique_lock<std::mutex> lk(update_lock_);
  shutdown_flag_ = true;
  update_var_.notify_all();
  lk.unlock();
  if (transform_thread_.joinable()) {
    transform_thread_.join();
  }

  std::cout << "Mouse x range: " << minx << " - " << maxx << std::endl;
}

void GlProgramm::SetImage(Frame&& frame) {
  std::unique_lock<std::mutex> fr_lock(last_frames_lock_);
  last_frames_.push_back(std::move(frame));
  fr_lock.unlock();
  update_var_.notify_all();
}

void GlProgramm::SetEyeSwap(bool swap) {
  std::unique_lock<std::mutex> lk(update_lock_);
  swap_eyes_setting_ = swap;
}

void GlProgramm::SetViewPoint(float x_disp, float y_disp) {
  std::unique_lock<std::mutex> lk(update_lock_);
  x_angle_ = x_disp;
  y_angle_ = y_disp;

  // TODO отладка
  if (x_angle_ < minx) {
    minx = x_angle_;
  }
  if (x_angle_ > maxx) {
    maxx = x_angle_;
  }
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

  FrameBuffer left_scene, right_scene;
  if (!CreateFrameBuffer(left_scene) || !CreateFrameBuffer(right_scene)) {
    throw std::runtime_error("Can't initialize scene framebuffers");
  }

  if (!CreateShaderProgram("split", split_program_)) {
    throw std::runtime_error("Can't create split program");
  }

  if (!CreateShaderProgram("halfcilinder", half_cilinder_program_)) {
    throw std::runtime_error("Can't create half cilinder program");
  }

  unsigned int output_program;
  if (!CreateShaderProgram("output", output_program)) {
    throw std::runtime_error("Can't create output program");
  }

  if (!CreateCubeVertex(cube_vertex_)) {
    throw std::runtime_error("Can't create cube scene");
  }

  if (!CreateFlatVertex(flat_vertex_)) {
    throw std::runtime_error("Can't create flat scene");
  }

  // Проекционная матрица на квадратное поле зрения
  projection_matrix_ = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 3.0f);

  // Входная текстура из проигрывателя
  GLuint tx;
  glGenTextures(1, &tx);
  glBindTexture(GL_TEXTURE_2D, tx);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);

  while (true) {
    bool swap_eyes;  //!< Локальное значение настройки swap_eyes_setting_
    float x_angle, y_angle;
    std::unique_lock<std::mutex> lk(update_lock_);
    if (shutdown_flag_) {
      break;
    }
    update_var_.wait(lk);
    if (shutdown_flag_) {
      break;
    }
    swap_eyes = swap_eyes_setting_;
    x_angle = x_angle_;
    y_angle = y_angle_;
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

    if (swap_eyes) {
      SplitScreen(tx, right_eye, left_eye, width, align_width);
    } else {
      SplitScreen(tx, left_eye, right_eye, width, align_width);
    }

    glm::mat4 rx =
        glm::rotate(glm::mat4(1.0f), x_angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rxy = glm::rotate(rx, y_angle, glm::vec3(1.0f, 0.0f, 0.0f));

    auto transform = projection_matrix_ * rxy;

    HalfCilinder(left_eye, left_scene, transform);
    HalfCilinder(right_eye, right_scene, transform);

    int scrw, scrh;
    screen_->GetFrameSize(scrw, scrh);
    glViewport(0, 0, scrw, scrh);

    glUseProgram(output_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, left_scene.texture);
    GLint loc = glGetUniformLocation(output_program, "left_image");
    glUniform1i(loc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, right_scene.texture);
    loc = glGetUniformLocation(output_program, "right_image");
    glUniform1i(loc, 1);
    glActiveTexture(GL_TEXTURE0);

    //    glBindTexture(GL_TEXTURE_2D, left_eye.texture);
    glBindVertexArray(flat_vertex_.array_id);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, flat_vertex_.array_size);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    screen_->DisplayBuffer();
  }

  glDeleteTextures(1, &tx);

  DeleteFrameBuffer(left_eye);
  DeleteFrameBuffer(right_eye);
  DeleteFrameBuffer(left_scene);
  DeleteFrameBuffer(right_scene);
  DeleteShaderProgram(split_program_);
  DeleteShaderProgram(half_cilinder_program_);
  DeleteShaderProgram(output_program);

  DeleteVertex(cube_vertex_);
  DeleteVertex(flat_vertex_);
}

void GlProgramm::SplitScreen(unsigned int texture, FrameBuffer& left,
    FrameBuffer& right, unsigned int width, unsigned int aligned_width) {
  // Разделим текстуру на две
  GLint loc;

  // Левая
  glBindFramebuffer(GL_FRAMEBUFFER, left.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glUseProgram(split_program_);

  loc = glGetUniformLocation(split_program_, "part_index");
  glUniform1i(loc, 0);

  loc = glGetUniformLocation(split_program_, "image_width");
  glUniform1f(loc, float(width) / aligned_width);

  glBindTexture(GL_TEXTURE_2D, texture);
  glBindVertexArray(flat_vertex_.array_id);
  glDrawArrays(GL_TRIANGLES, 0, flat_vertex_.array_size);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Правая
  glBindFramebuffer(GL_FRAMEBUFFER, right.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glUseProgram(split_program_);
  glBindTexture(GL_TEXTURE_2D, texture);

  loc = glGetUniformLocation(split_program_, "part_index");
  glUniform1i(loc, 1);

  glBindVertexArray(flat_vertex_.array_id);
  glDrawArrays(GL_TRIANGLES, 0, flat_vertex_.array_size);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlProgramm::HalfCilinder(const FrameBuffer& in_buffer,
    const FrameBuffer& out_buffer, const glm::mat4& transform) {
  glBindFramebuffer(GL_FRAMEBUFFER, out_buffer.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(half_cilinder_program_);
  glBindTexture(GL_TEXTURE_2D, in_buffer.texture);
  auto tr_var = glGetUniformLocation(half_cilinder_program_, "transformation");
  glUniformMatrix4fv(tr_var, 1, GL_TRUE, glm::value_ptr(transform));

  glBindVertexArray(cube_vertex_.array_id);
  //      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  glDrawArrays(GL_TRIANGLES, 0, cube_vertex_.array_size);
  //      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GlProgramm::DeleteVertex(VertexArray& vertex) {
  glDeleteVertexArrays(1, &vertex.array_id);
  vertex.array_id = 0;
  vertex.array_size = 0;
}

bool GlProgramm::CreateCubeVertex(VertexArray& vertex) {
  const GLuint kVertexAmount = 36;
  GLfloat cube[kVertexAmount * 3] = {// clang-format off
    // Far face
    // x   y      z
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
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
  // clang-format on

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
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

bool GlProgramm::CreateFlatVertex(VertexArray& vertex) {
  const GLint kVertexAmount = 6;
  // clang-format off
  GLfloat flat[kVertexAmount * 3] = {
    -1.0f, -1.0f,  0.0f,
    -1.0f,  1.0f,  0.0f,
     1.0f, -1.0f,  0.0f,
    -1.0f,  1.0f,  0.0f,
     1.0f, -1.0f,  0.0f,
     1.0f,  1.0f,  0.0f
  };
  // clang-format on

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(flat), flat, GL_STATIC_DRAW);
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
