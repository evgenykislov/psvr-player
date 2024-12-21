#include "rotation.h"


const double kPi = 3.1415926535897932384626433832795;
const double kNearZeroLength2 =
    1.0e-8;  //!< Длина очень короткого вектора. При расчётах углов означает
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
  double right_angle = atan2(view_.x, view_.z);
  double top_angle;
  if (view_.y >= 1.0) {
    top_angle = kPi / 2.0;
  } else if (view_.y <= -1.0) {
    top_angle = -kPi / 2.0;
  } else {
    top_angle = asin(view_.y);
  }

  auto horz = glm::cross(view_, vec3d(0.0, 1.0, 0.0));
  double clock_angle;
  if (glm::length2(horz) < kNearZeroLength2) {
    clock_angle = 0.0;
  } else {
    clock_angle = kPi / 2.0 - glm::angle(glm::normalize(horz), tip_);
  }

  // Последовательность поворотов: кручение (roll_angle), подъём (top_angle),
  // в горизонтальной плоскости (right_angle)
  glm::mat4 r1 = glm::rotate(
      glm::mat4(1.0f), float(clock_angle), glm::vec3(0.0f, 0.0f, 1.0f));
  glm::mat4 r2 =
      glm::rotate(r1, float(top_angle), glm::vec3(-1.0f, 0.0f, 0.0f));
  glm::mat4 r3 =
      glm::rotate(r2, float(right_angle), glm::vec3(0.0f, 1.0f, 0.0f));
  rot_mat = r3;
}
