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

void GenerateLeftRight(track& newtrack) {
  newtrack.clear();
  const float dangle = 0.1;

  for (float angle = -90; angle < 90; angle += dangle) {
    Point p;
    p.Helmet.x = cos(glm::radians(angle));
    p.Helmet.y = 0.0;
    p.Helmet.z = sin(glm::radians(angle));
    p.Tip = glm::vec3(0.0, 1.0, 0.0);
    newtrack.push_back(p);
  }
}


/*! Подсчёт угловых перемещений от точки p1 к точке p2 */
void GetAngles(Point p1, Point p2, double& right_angle, double& top_angle,
    double& clock_angle) {
  // right angle
  auto n1 = glm::normalize(glm::cross(p1.Helmet, p1.Tip));
  auto n2 = glm::normalize(glm::cross(p2.Helmet, p1.Tip));
  right_angle = glm::degrees(glm::acos(glm::dot(n1, n2)));

  top_angle = 0.0f;
  clock_angle = 0.0f;
}

double Distance(glm::vec3 a, glm::vec3 b) {
  auto c = a - b;
  return glm::length(c);
}

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

    auto view = m * glm::vec4(it->Helmet, 1.0);
    auto tip = m * glm::vec4(it->Tip, 1.0);

    //    std::cout << "Distance: " << Distance(view, base.Helmet) << ", " <<
    //    Distance(tip, base.Tip) << std::endl;

    EXPECT_LE(Distance(view, base.Helmet), kSuitableNearZeroLength);
    EXPECT_LE(Distance(tip, base.Tip), kSuitableNearZeroLength);
  }
}


TEST(LeftRightView, Mathematics) {
  track t;
  GenerateLeftRight(t);
  CheckTrack(t);
}

/*
TEST(RotationView, Mathematics) {
  track t;
  GenerateRotation(t);

  EXPECT_FALSE(t.empty());
}
*/
