#include "rotation.h"


#define DEBUG_OUTPUT


#ifdef DEBUG_OUTPUT
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#endif


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
  auto rv1 = glm::rotate(right, glm::radians(right1), tip_);
  auto fv2 = glm::rotate(fv1, glm::radians(-top1), rv1);
  auto uv1 = glm::rotate(tip_, glm::radians(-top1), rv1);
  //  auto rv2 = glm::rotate(rv1, glm::radians(-clock1), fv2);
  auto uv2 = glm::rotate(uv1, glm::radians(-clock1), fv2);

  view_ = glm::normalize(fv2);
  tip_ = glm::normalize(uv2);

#ifdef DEBUG_OUTPUT
  std::cout << "Rotation: apply " << right1 << ", " << top1 << ", " << clock1
            << std::endl;
  std::cout << "  result: view " << glm::to_string(view_) << ", tip "
            << glm::to_string(tip_) << std::endl;
  std::cout << "  view-tip angle: "
            << glm::degrees(glm::acos(glm::dot(view_, tip_))) << std::endl;

#endif
}


void Rotation::GetSummRotation(glm::mat4& rot_mat) {
  std::lock_guard<std::mutex> l(data_lock_);

  vec3d base_view(0.0, 0.0, 1.0);
  vec3d base_tip(0.0, 1.0, 0.0);
  glm::dmat4 m(1);

  auto view_axis = glm::cross(base_view, view_);
  auto base_angle = RadAngle(view_axis, base_view, view_);
  auto fix_base_tip = glm::rotate(base_tip, base_angle, view_axis);
  auto tip_axis = glm::cross(fix_base_tip, tip_);
  auto tip_angle = RadAngle(tip_axis, fix_base_tip, tip_);
  m = glm::rotate(m, tip_angle, tip_axis);
  m = glm::rotate(m, base_angle, view_axis);

  rot_mat = m;
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
