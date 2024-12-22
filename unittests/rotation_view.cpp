#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <gtest/gtest.h>

#include "../psvrplayer/rotation.h"


using vec3d = glm::vec<3, double, glm::defaultp>;

struct Point {
  vec3d Helmet;  // Direction of helmet view
  vec3d Tip;     // Direction of up-side of helment. Length is 1.0
};

using track = std::vector<Point>;

const double kSuitableNearZeroLength = 1.0e-6;

/*
void GenerateRotation(track& newtrack) {
  const float dangle = 90.0;
  glm::vec3 polar(0.0, -1.0, 10.0);
  glm::mat4 rot(1);
  rot = glm::rotate(rot, dangle, glm::normalize(glm::vec3(0.0, 1.0, 1.0)));

  newtrack.clear();
  glm::vec3 dir(0.0, 0.0, 1.0);
  for (double angle = 0.0; angle < 360; angle += dangle) {
    // proc
    auto n1 = glm::cross(dir, polar);
    n1 = glm::normalize(n1);
    auto tip = glm::cross(dir, n1);
    tip = glm::normalize(tip);

    Point p;
    p.Helmet = dir;
    p.Tip = tip;

    newtrack.push_back(p);

    dir = glm::vec3(rot * glm::vec4(dir, 1.0));
  }
}
*/


/*! Сосчитаем угол поворота между векторами в заданной плоскости. Поворот
считается по часовой стрелки с точки зрения вектора нормали. Система
координат: х - вправо, y - вверх, z - вперёд от нас \param n плоскость, в
которой считаются углы. Задаётся нормалью \param v1 вектор от которого считается
поворот \param v2 вектор в сторону которого считается поворот \return угол
поворота в градусах (от -180 до +180) */
double Angle(const vec3d& n, const vec3d& v1, const vec3d& v2) {
  auto p1 = glm::cross(n, v1);
  auto p2 = glm::cross(n, v2);
  auto raw_d = glm::dot(p1, p2);
  auto cos2 = (raw_d * raw_d) / glm::length2(p1) / glm::length2(p2);

  double angle = 0.0;
  if (cos2 >= 1.0) {
    angle = 0.0;
  } else {
    angle = glm::degrees(glm::acos(2.0 * cos2 - 1.0)) / 2.0;
    if (raw_d < 0) {
      angle = 180.0 - angle;
    }
  }

  if (glm::dot(glm::cross(p1, p2), n) < 0) {
    return -angle;
  }

  return angle;
}


/*! Подсчёт угловых перемещений от точки p1 к точке p2 */
void GetAngles(Point p1, Point p2, double& right_angle, double& top_angle,
    double& clock_angle) {
  right_angle = Angle(p1.Tip, p1.Helmet, p2.Helmet);
  auto top_axis = glm::normalize(glm::cross(p1.Tip, p1.Helmet));
  top_angle = -Angle(top_axis, p1.Helmet, p2.Helmet);
  clock_angle = Angle(p1.Helmet, p1.Tip, p2.Tip);
}


/*! Считает расстояние между двумя векторами. Фактически это длина вектора
 * разности */
double Distance(glm::vec3 a, glm::vec3 b) {
  auto c = a - b;
  return glm::length(c);
}


/*! Проверяет трек точек на правильность расчёта */
void CheckTrack(const track& t) {
  EXPECT_GE(t.size(), 2);

  auto base = t[0];
  Rotation rt;

  for (auto it = ++t.begin(); it != t.end(); ++it) {
    double r, t, c;
    GetAngles(*(it - 1), *it, r, t, c);

    rt.Rotate(r, t, c);

    glm::mat4 m;
    rt.GetSummRotation(m);

    auto view = m * glm::vec4(base.Helmet, 1.0);
    auto tip = m * glm::vec4(base.Tip, 1.0);

    //    std::cout << "Distance: " << Distance(view, it->Helmet) << ", " <<
    //    Distance(tip, it->Tip) << std::endl;

    EXPECT_LE(Distance(view, it->Helmet), kSuitableNearZeroLength);
    EXPECT_LE(Distance(tip, it->Tip), kSuitableNearZeroLength);
  }
}


TEST(RightView, Mathematics) {
  track t;
  t.clear();
  const float dangle = 0.1;

  for (float angle = 0; angle < 90; angle += dangle) {
    Point p;
    p.Helmet.x = sin(glm::radians(angle));
    p.Helmet.y = 0.0;
    p.Helmet.z = cos(glm::radians(angle));
    p.Tip = glm::vec3(0.0, 1.0, 0.0);
    t.push_back(p);
  }
  CheckTrack(t);
}


TEST(LeftView, Mathematics) {
  track t;
  t.clear();
  const float dangle = 0.1;

  for (float angle = 0; angle > -90; angle -= dangle) {
    Point p;
    p.Helmet.x = sin(glm::radians(angle));
    p.Helmet.y = 0.0;
    p.Helmet.z = cos(glm::radians(angle));
    p.Tip = glm::vec3(0.0, 1.0, 0.0);
    t.push_back(p);
  }
  CheckTrack(t);
}


TEST(TopView, Mathematics) {
  track t;
  t.clear();
  const float dangle = 0.1;

  for (float angle = 0; angle < 85; angle += dangle) {
    Point p;
    p.Helmet.x = 0.0;
    p.Helmet.y = sin(glm::radians(angle));
    p.Helmet.z = cos(glm::radians(angle));
    p.Tip.x = 0.0;
    p.Tip.y = cos(glm::radians(angle));
    p.Tip.z = -sin(glm::radians(angle));
    t.push_back(p);
  }
  CheckTrack(t);
}


TEST(DownView, Mathematics) {
  track t;
  t.clear();
  const float dangle = 0.1;

  for (float angle = 0; angle > -85; angle -= dangle) {
    Point p;
    p.Helmet.x = 0.0;
    p.Helmet.y = sin(glm::radians(angle));
    p.Helmet.z = cos(glm::radians(angle));
    p.Tip.x = 0.0;
    p.Tip.y = cos(glm::radians(angle));
    p.Tip.z = -sin(glm::radians(angle));
    t.push_back(p);
  }
  CheckTrack(t);
}


TEST(ClockView, Mathematics) {
  track t;
  t.clear();
  const float dangle = 0.1;

  for (float angle = 0; angle < 180; angle += dangle) {
    Point p;
    p.Helmet.x = 0.0;
    p.Helmet.y = 0.0;
    p.Helmet.z = 1.0;
    p.Tip.x = sin(glm::radians(angle));
    p.Tip.y = cos(glm::radians(angle));
    p.Tip.z = 0.0;
    t.push_back(p);
  }
  CheckTrack(t);
}


TEST(CClockView, Mathematics) {
  track t;
  t.clear();
  const float dangle = 0.1;

  for (float angle = 0; angle > -180; angle -= dangle) {
    Point p;
    p.Helmet.x = 0.0;
    p.Helmet.y = 0.0;
    p.Helmet.z = 1.0;
    p.Tip.x = sin(glm::radians(angle));
    p.Tip.y = cos(glm::radians(angle));
    p.Tip.z = 0.0;
    t.push_back(p);
  }
  CheckTrack(t);
}

/*
TEST(RotationView, Mathematics) {
  track t;
  GenerateRotation(t);

  EXPECT_FALSE(t.empty());
}
*/
