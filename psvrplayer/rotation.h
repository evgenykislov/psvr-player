#ifndef ROTATION_H
#define ROTATION_H

#include <mutex>

#include <glm/gtx/vector_angle.hpp>


class Rotation {
 public:
  Rotation();
  ~Rotation() = default;

  /*! Сбросить координаты к начальному значению: смотрим вперёд горизонтально */
  void Reset();

  /*! Повернуть обзор шлема направо, вверх и самовращение. Поворот задаётся в
  градусах, порядок считается неважным, т.к. приращения угла (должны быть)
  небольшие. \param right поворот вправо (-влево) относительно самого шлема
  \param top запрокидывание вверх (-вниз) относительно самого шлема
  \param clock кручение шлема по часовой стрелке (-против часовой) */
  void Rotate(double right, double top, double clock);

  /*! Выдать суммарное (общее) вращение шлема относительно базового расположения
  (смотрим вперёд горизонтально) \param rot_mat выдаваемая матрица поворота */
  void GetSummRotation(glm::mat4& rot_mat);

 private:
  Rotation(const Rotation&) = default;
  Rotation(Rotation&&) = default;
  Rotation& operator=(const Rotation&) = default;
  Rotation& operator=(Rotation&&) = default;

  using vec3d = glm::vec<3, double, glm::defaultp>;

  vec3d view_;  //!< Вектор куда смотрит сам шлем (мировые координаты)
  vec3d tip_;  //!< Вектор куда смотрит верх шлема (мировые координаты)
  std::mutex data_lock_;
};

#endif  // ROTATION_H
