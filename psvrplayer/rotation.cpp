#include "rotation.h"


const double kPi = 3.1415926535897932384626433832795;
const double kNearZeroLength =
    1.0e-4;  //!< Длина очень короткого вектора. При расчётах углов означает
             //!< взгляд ровно вверх или вниз


Rotation::Rotation() { Reset(); }

void Rotation::Reset() {
  std::lock_guard<std::mutex> l(data_lock_);
  view_ = vec3d(0.0, 0.0, 1.0);
  tip_ = vec3d(0.0, 1.0, 0.0);
}

void Rotation::Rotate(double right1, double top1, double clock1) {
  std::lock_guard<std::mutex> l(data_lock_);
  auto right = -glm::cross(view_, tip_);
  auto fv1 = glm::rotate(view_, glm::radians(right1), tip_);
  //  auto rv1 = glm::rotate(right, glm::radians(right1), tip_);
  auto fv2 = glm::rotate(fv1, glm::radians(-top1), right);
  auto uv1 = glm::rotate(tip_, glm::radians(-top1), right);
  //  auto rv2 = glm::rotate(rv1, glm::radians(-clock1), view_);
  auto uv2 = glm::rotate(uv1, glm::radians(-clock1), view_);

  view_ = glm::normalize(fv2);
  tip_ = glm::normalize(uv2);
}


void Rotation::GetSummRotation(glm::mat4& rot_mat) {
  double clock_angle = 0.0;
  double right_angle = 0.0;
  double top_angle = 0.0;

  std::lock_guard<std::mutex> l(data_lock_);

  // Отработаем случай, когда взгляд идёт ровно вверх или ровно вниз
  vec3d zenith(0.0, 1.0, 0.0);
  auto top_dev = glm::cross(view_, vec3d(0.0, 1.0, 0.0));
  if (glm::length(top_dev) < kNearZeroLength) {
    // Взгляд ровно верх или вниз
    clock_angle = 0.0;  // Считаем, что шлем не наклонён
    if (view_.y > 0.0) {
      // Смотрим ровно вверх
      // Горизонтальный угол считаем как противоположный для вершины
      right_angle = atan2(-tip_.x, -tip_.z);
      top_angle = kPi / 2.0;
    } else {
      // Смотрим ровно вниз
      right_angle = atan2(tip_.x, tip_.z);
      top_angle = -kPi / 2.0;
    }
  } else {
    right_angle = atan2(view_.x, view_.z);
    top_angle = asin(view_.y);
    clock_angle = RadAngle(view_, tip_, zenith);
  }

  // Последовательность (обратная) поворотов: кручение (roll_angle), подъём
  // (top_angle), в горизонтальной плоскости (right_angle)
  glm::mat4 r1 = glm::rotate(
      glm::mat4(1.0f), float(clock_angle), glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 r2 =
      glm::rotate(r1, float(top_angle), glm::vec3(-1.0f, 0.0f, 0.0f));
  glm::mat4 r3 =
      glm::rotate(r2, float(right_angle), glm::vec3(0.0f, 1.0f, 0.0f));
  rot_mat = r3;
}


double Rotation::RadAngle(const vec3d& n, const vec3d& v1, const vec3d& v2) {
  auto p1 = glm::cross(n, v1);
  auto p2 = glm::cross(n, v2);
  auto raw_d = glm::dot(p1, p2);
  auto cos2 = (raw_d * raw_d) / glm::length2(p1) / glm::length2(p2);

  double angle = 0.0;
  if (cos2 >= 1.0) {
    angle = 0.0;
  } else {
    angle = glm::acos(2.0 * cos2 - 1.0) / 2.0;
    if (raw_d < 0) {
      angle = kPi - angle;
    }
  }

  if (glm::dot(glm::cross(p1, p2), n) < 0) {
    return -angle;
  }

  return angle;
}
