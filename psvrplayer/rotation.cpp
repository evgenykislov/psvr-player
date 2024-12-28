#include "rotation.h"


#define DEBUG_OUTPUT


#ifdef DEBUG_OUTPUT
#include <iostream>
#include <glm/gtx/string_cast.hpp>
#endif


const double kPi = 3.1415926535897932384626433832795;
const double kZeroVectorLength2 = 1.0e-25;

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
}


void Rotation::GetSummRotation(glm::mat4& rot_mat) {
  std::lock_guard<std::mutex> l(data_lock_);

  vec3d base_view(0.0, 0.0, 1.0);
  vec3d base_tip(0.0, 1.0, 0.0);
  glm::dmat4 m(1);

  auto fix_base_tip = base_tip;
  const double kZeroAngle = 0.0;
  double base_angle = kZeroAngle;
  double tip_angle = kZeroAngle;
  auto view_axis = glm::cross(base_view, view_);
  auto view_axis_length2 = glm::length2(view_axis);
  if (view_axis_length2 > kZeroVectorLength2) {
    base_angle = RadAngle(view_axis, base_view, view_);
    fix_base_tip = glm::rotate(base_tip, base_angle, view_axis);
  }
  auto tip_axis = glm::cross(fix_base_tip, tip_);
  auto tip_axis_length2 = glm::length2(tip_axis);
  if (tip_axis_length2 > kZeroVectorLength2) {
    tip_angle = RadAngle(tip_axis, fix_base_tip, tip_);
  }

  if (tip_angle != kZeroAngle) {
    m = glm::rotate(m, tip_angle, tip_axis);
  }
  if (base_angle != kZeroAngle) {
    m = glm::rotate(m, base_angle, view_axis);
  }

  rot_mat = m;
}


double Rotation::RadAngle(const vec3d& n, const vec3d& v1, const vec3d& v2) {
  auto v1length2 = glm::length2(v1);
  auto v2length2 = glm::length2(v2);
  if (v1length2 < kZeroVectorLength2 || v2length2 < kZeroVectorLength2) {
    // Угол между вектором и точкой всегда нулевой
    return 0.0;
  }

  // Вектора и длины, для которых считается угол
  auto p1 = v1;
  auto p2 = v2;
  auto p1length2 = v1length2;
  auto p2length2 = v2length2;
  auto nlength2 = glm::length2(n);
  if (nlength2 > kZeroVectorLength2) {
    p1 = glm::cross(n, v1);
    p2 = glm::cross(n, v2);
    p1length2 = glm::length2(p1);
    p2length2 = glm::length2(p2);
  }  // else ... При нулевой нормали считаем обычный угол между векторами

  if (p1length2 < kZeroVectorLength2 || p2length2 < kZeroVectorLength2) {
    // Один из векторов параллелен нормали. Фактически это вычисление угла между
    // вектором и точкой
    return 0.0;
  }

  auto raw_d = glm::dot(p1, p2);
  auto cos2 = (raw_d * raw_d) / p1length2 / p2length2;

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
