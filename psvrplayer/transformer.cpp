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
#include <glm/gtx/rotate_vector.hpp>

#include "frame_buffer.h"
#include "play_screen.h"
#include "shader_program.h"
#include "vr_helmet.h"
#include "shaders/flat.vert.h"
#include "shaders/flat.frag.h"
#include "shaders/halfcilinder.vert.h"
#include "shaders/halfcilinder.frag.h"
#include "shaders/output.vert.h"
#include "shaders/output.frag.h"
#include "shaders/split.vert.h"
#include "shaders/split.frag.h"


/*! Массив вершин для отрисовки, зарегистрированный как объект в opengl */
struct VertexArray {
  unsigned int array_id;  //!< Идентификатор массива вершин
  unsigned int array_size;  //!< Размер массива, количество вершин в отрисовку
};


// const double kPi = 3.1415926535897932384626433832795;


class GlProgramm: public Transformer {
 public:
  GlProgramm(TransformerScheme scheme, StreamsScheme streams,
      IPlayScreenPtr screen, std::shared_ptr<IHelmet> helmet);
  ~GlProgramm();

  void SetImage(Frame&& frame) override;
  void SetEyeSwap(bool swap) override;
  void SetViewPoint(float x_disp, float y_disp) override;
  void SetEyesDistance(int distance) override;

 private:
  GlProgramm() = delete;
  GlProgramm(const GlProgramm&) = delete;
  GlProgramm(GlProgramm&&) = delete;
  GlProgramm& operator=(const GlProgramm&) = delete;
  GlProgramm& operator=(GlProgramm&&) = delete;

  // TODO description???
  enum SplitScheme { kSplitSingleImage, kSplitLeftRight, kSplitUpDown };

  // Описание всех параметров для построения сцены
  // Заполняется и используется только в потоке трансформации и отображения
  struct SceneParameters {
    // Настройки
    bool swap_eyes;  // Настройка: поменять изображения для левого-правого глаз
    TransformerScheme scheme;
    float eyes_correction;

    // Вход-Выход
    GLuint input_texture;  // Номер текстуры входного изображения
    int width, height, align_width;  // Размеры входной текстуры
    double right_angle, top_angle, roll_angle;  // Поворот шлема
    FrameBuffer left_scene,
        right_scene;  // Кадровые буфера с выходными изображениями


    // Вспомогательное
    FrameBuffer left_eye,
        right_eye;  // Вспомогательные кадровые буфера для отрисовки сцены
  };

  std::thread transform_thread_;
  TransformerScheme scheme_settings_;
  StreamsScheme streams_settings_;
  IPlayScreenPtr screen_;
  std::shared_ptr<IHelmet> helmet_;

  std::vector<Frame> last_frames_;
  std::mutex last_frames_lock_;
  bool swap_eyes_setting_;  //!< Настройка по смене порядка изображений для
                            //!< глаз. Настройка под блокировкой update_lock_
  float eyes_correction_;  //!< Корректировка глазного расстояния. Под
                           //!< блокировкой update_lock_
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
  unsigned int flat_program_;
  glm::mat4 projection_matrix_;  //!< Проекционная матрица
  VertexArray cube_vertex_;  //!< Вершины для кубической сцены (формирование
                             //!< полусфер и т.д.)
  VertexArray flat_vertex_;  //!< Вершины для плоской сцены (вывод изображений)

  void Processing();

  // TODO ???
  void SplitScreen(unsigned int texture, const FrameBuffer& left,
      const FrameBuffer& right, unsigned int width, unsigned int aligned_width);

  /*! Отрисовать входной буфер (in_buffer) натянутым на цилиндрическую
  поверхность охватом в 180 градусов и результат выдать в выходной буфер
  (out_buffer). Также применить повороты из матрицы трансформации (transform) */
  void HalfCilinder(const FrameBuffer& in_buffer, const FrameBuffer& out_buffer,
      const glm::mat4& transform);

  /*! Отрисовать входной буфер (in_buffer) натянутым на плоскость и
  результат выдать в выходной буфер (out_buffer). Также применить повороты из
  матрицы трансформации (transform)
  \param width2height отношение ширины к высоте для рендеринга изображения */
  void RenderFlat(const FrameBuffer& in_buffer, const FrameBuffer& out_buffer,
      const glm::mat4& transform, double width2height);


  /*! Удалить массив вершин */
  void DeleteVertex(VertexArray& vertex);

  /*! Создать/зарегистрировать массив вершин для отрисовки куба */
  bool CreateCubeVertex(VertexArray& vertex);

  // TODO ???
  bool CreateFlatVertex(VertexArray& vertex);

  void SchemeLeftRight180(const SceneParameters& params);
  void SchemeSingleImage(const SceneParameters& params);

  /*! Разбор кадра как плоский 3D фильм
  \param params параметры сцены и входные/выходные текстуры (через индексы) */
  void SchemeFlat3D(const SceneParameters& params);
};

Transformer* CreateTransformer(TransformerScheme scheme, StreamsScheme streams,
    IPlayScreenPtr screen, std::shared_ptr<IHelmet> helmet) {
  try {
    return new GlProgramm(scheme, streams, screen, helmet);
  } catch (...) {
  }
  return nullptr;
}

GlProgramm::GlProgramm(TransformerScheme scheme, StreamsScheme streams,
    IPlayScreenPtr screen, std::shared_ptr<IHelmet> helmet)
    : swap_eyes_setting_(false),
      eyes_correction_(0.0f),
      split_program_(0),
      half_cilinder_program_(0),
      flat_program_(0) {
  scheme_settings_ = scheme;
  streams_settings_ = streams;
  screen_ = screen;
  helmet_ = helmet;
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

void GlProgramm::SetEyesDistance(int distance) {
  std::unique_lock<std::mutex> lk(update_lock_);
  eyes_correction_ = (66 - distance) / 72.0f;
}

void GlProgramm::Processing() {
  SceneParameters params;

  screen_->MakeScreenCurrent();
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Can't initialize Glad context. Maybe a logic error: make "
                 "current context for window first"
              << std::endl;
    throw std::runtime_error("Can't initialize Glad");
  }

  if (!CreateFrameBuffer(params.left_eye)) {
    throw std::runtime_error("Can't initialize left framebuffer");
  }
  if (!CreateFrameBuffer(params.right_eye)) {
    throw std::runtime_error("Can't initialize right framebuffer");
  }

  if (!CreateFrameBuffer(params.left_scene) ||
      !CreateFrameBuffer(params.right_scene)) {
    throw std::runtime_error("Can't initialize scene framebuffers");
  }

  if (!CreateShaderProgram(split_program_, shaders_split_vert,
          shaders_split_vert_len, shaders_split_frag, shaders_split_frag_len)) {
    throw std::runtime_error("Can't create split program");
  }

  if (!CreateShaderProgram(half_cilinder_program_, shaders_halfcilinder_vert,
          shaders_halfcilinder_vert_len, shaders_halfcilinder_frag,
          shaders_halfcilinder_frag_len)) {
    throw std::runtime_error("Can't create half cilinder program");
  }

  if (!CreateShaderProgram(flat_program_, shaders_flat_vert,
          shaders_flat_vert_len, shaders_flat_frag, shaders_flat_frag_len)) {
    throw std::runtime_error("Can't create flat program");
  }

  unsigned int output_program;
  if (!CreateShaderProgram(output_program, shaders_output_vert,
          shaders_output_vert_len, shaders_output_frag,
          shaders_output_frag_len)) {
    throw std::runtime_error("Can't create output program");
  }

  if (!CreateCubeVertex(cube_vertex_)) {
    throw std::runtime_error("Can't create cube scene");
  }

  if (!CreateFlatVertex(flat_vertex_)) {
    throw std::runtime_error("Can't create flat scene");
  }

  // Проекционная матрица на квадратное поле зрения
  const float kDistorsionCompensation =
      2.0f;  // Компенсация сужения изображения к центру (в 2 раза) при
             // компенсации дисторсии
  projection_matrix_ = glm::perspective(
      glm::radians(60.0f * kDistorsionCompensation), 1.0f, 0.1f, 3.0f);

  // Входная текстура из проигрывателя
  glGenTextures(1, &params.input_texture);
  glBindTexture(GL_TEXTURE_2D, params.input_texture);
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
    params.swap_eyes = swap_eyes_setting_;
    params.scheme = scheme_settings_;
    params.eyes_correction = eyes_correction_;
    lk.unlock();

    if (helmet_) {
      helmet_->GetViewPoint(
          params.right_angle, params.top_angle, params.roll_angle);
    } else {
      params.right_angle = params.top_angle = params.roll_angle = 0.0;
    }

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

    glBindTexture(GL_TEXTURE_2D, params.input_texture);
    frame.GetSizes(&params.width, &params.height, &params.align_width, nullptr);
    size_t data_size;
    void* data = frame.GetData(data_size);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, params.align_width, params.height, 0,
        GL_BGRA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    ReleaseFrame(std::move(frame));

    switch (params.scheme) {
      case kLeftRight180:
        SchemeLeftRight180(params);
        break;
      case kSingleImage:
        SchemeSingleImage(params);
        break;
      case kFlat3D:
        SchemeFlat3D(params);
        break;
      default:
        assert(false);
    }

    int scrw, scrh;
    screen_->GetFrameSize(scrw, scrh);
    glViewport(0, 0, scrw, scrh);

    glUseProgram(output_program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, params.left_scene.texture);
    GLint loc = glGetUniformLocation(output_program, "left_image");
    glUniform1i(loc, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, params.right_scene.texture);
    loc = glGetUniformLocation(output_program, "right_image");
    glUniform1i(loc, 1);
    glActiveTexture(GL_TEXTURE0);
    // eyes correction
    loc = glGetUniformLocation(output_program, "eyes_correction");
    glUniform1f(loc, params.eyes_correction);

    //    glBindTexture(GL_TEXTURE_2D, left_eye.texture);
    glBindVertexArray(flat_vertex_.array_id);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, flat_vertex_.array_size);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    screen_->DisplayBuffer();
  }

  glDeleteTextures(1, &params.input_texture);

  DeleteFrameBuffer(params.left_eye);
  DeleteFrameBuffer(params.right_eye);
  DeleteFrameBuffer(params.left_scene);
  DeleteFrameBuffer(params.right_scene);
  DeleteShaderProgram(flat_program_);
  DeleteShaderProgram(split_program_);
  DeleteShaderProgram(half_cilinder_program_);
  DeleteShaderProgram(output_program);

  DeleteVertex(cube_vertex_);
  DeleteVertex(flat_vertex_);
}

void GlProgramm::SplitScreen(unsigned int texture, const FrameBuffer& left,
    const FrameBuffer& right, unsigned int width, unsigned int aligned_width) {
  // Разделим текстуру на две
  GLint loc;
  GLint left_sc, right_sc;

  switch (streams_settings_) {
    case kLeftRightStreams:
      left_sc = 0;
      right_sc = 1;
      break;
    case kUpDownStreams:
      left_sc = 2;
      right_sc = 3;
      break;
    case kSingleStream:
      left_sc = 100;
      right_sc = 100;
      break;
  }

  // Левая
  glBindFramebuffer(GL_FRAMEBUFFER, left.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glUseProgram(split_program_);

  loc = glGetUniformLocation(split_program_, "part_index");
  glUniform1i(loc, left_sc);

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
  glUniform1i(loc, right_sc);

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


void GlProgramm::RenderFlat(const FrameBuffer& in_buffer,
    const FrameBuffer& out_buffer, const glm::mat4& transform,
    double width2height) {
  glBindFramebuffer(GL_FRAMEBUFFER, out_buffer.buffer);
  glViewport(0, 0, FrameBuffer::texture_size, FrameBuffer::texture_size);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(flat_program_);
  glBindTexture(GL_TEXTURE_2D, in_buffer.texture);
  auto tr_var = glGetUniformLocation(flat_program_, "transformation");
  glUniformMatrix4fv(tr_var, 1, GL_TRUE, glm::value_ptr(transform));
  tr_var = glGetUniformLocation(flat_program_, "width2height");
  assert(tr_var != -1);
  glUniform1f(tr_var, (float)width2height);

  glBindVertexArray(cube_vertex_.array_id);
  glDrawArrays(GL_TRIANGLES, 0, cube_vertex_.array_size);
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

void GlProgramm::SchemeLeftRight180(const SceneParameters& params) {
  if (params.swap_eyes) {
    SplitScreen(params.input_texture, params.right_eye, params.left_eye,
        params.width, params.align_width);
  } else {
    SplitScreen(params.input_texture, params.left_eye, params.right_eye,
        params.width, params.align_width);
  }

  // Последовательность поворотов: кручение (roll_angle), подъём (top_angle),
  // в горизонтальной плоскости (right_angle)
  glm::mat4 r1 = glm::rotate(
      glm::mat4(1.0f), (float)params.roll_angle, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 r2 =
      glm::rotate(r1, float(params.top_angle), glm::vec3(-1.0f, 0.0f, 0.0f));
  glm::mat4 r3 =
      glm::rotate(r2, float(params.right_angle), glm::vec3(0.0f, 1.0f, 0.0f));

  auto transform = projection_matrix_ * r3;

  HalfCilinder(params.left_eye, params.left_scene, transform);
  HalfCilinder(params.right_eye, params.right_scene, transform);
}

void GlProgramm::SchemeSingleImage(const GlProgramm::SceneParameters& params) {
  SplitScreen(params.input_texture, params.left_scene, params.right_scene,
      params.width, params.align_width);
}

void GlProgramm::SchemeFlat3D(const GlProgramm::SceneParameters& params) {
  if (params.swap_eyes) {
    SplitScreen(params.input_texture, params.right_eye, params.left_eye,
        params.width, params.align_width);
  } else {
    SplitScreen(params.input_texture, params.left_eye, params.right_eye,
        params.width, params.align_width);
  }

  // Последовательность поворотов: кручение (roll_angle), подъём (top_angle),
  // в горизонтальной плоскости (right_angle)
  glm::mat4 r1 = glm::rotate(
      glm::mat4(1.0f), (float)params.roll_angle, glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 r2 =
      glm::rotate(r1, float(params.top_angle), glm::vec3(-1.0f, 0.0f, 0.0f));
  glm::mat4 r3 =
      glm::rotate(r2, float(params.right_angle), glm::vec3(0.0f, 1.0f, 0.0f));

  auto transform = projection_matrix_ * r3;

  auto w2h = double(params.width) / double(params.height);

  RenderFlat(params.left_eye, params.left_scene, transform, w2h);
  RenderFlat(params.right_eye, params.right_scene, transform, w2h);
}
