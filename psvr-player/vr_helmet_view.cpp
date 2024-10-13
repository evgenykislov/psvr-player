#include "vr_helmet_view.h"

#include <cassert>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <hidapi/hidapi.h>

#include "iniparser.h"


#define DEBUG_HELMET_VIEW

const double kPi = 3.1415926535897932384626433832795;

// TODO Detects home directory and generate correct configuration file name
const char kConfigFileName[] = "/tmp/psvrplayer.cfg";

PsvrHelmetView::PsvrHelmetView() {
  center_view_flag_ = true;
  last_sensor_time_ = std::numeric_limits<uint64_t>::max();

  std::unique_lock<std::mutex> vl(velo_lock_);
  auto dict = iniparser_load(kConfigFileName);
  if (dict) {
    right_velo_ = double(iniparser_getint64(dict, "Calibration:right", 0)) /
                  kFixedPointFactor;
    top_velo_ = double(iniparser_getint64(dict, "Calibration:top", 0)) /
                kFixedPointFactor;
    clock_velo_ = double(iniparser_getint64(dict, "Calibration:clock", 0)) /
                  kFixedPointFactor;
    iniparser_freedict(dict);
  } else {
    right_velo_ = 0.0;
    top_velo_ = 0.0;
    clock_velo_ = 0.0;
  }
  vl.unlock();
}


void PsvrHelmetView::OnSensorsData(
    double to_right, double to_top, double to_clockwork, uint64_t mcs_time) {
  if (last_sensor_time_ == std::numeric_limits<uint64_t>::max()) {
    last_sensor_time_ = mcs_time;
    return;
  }
  double ims = (mcs_time - last_sensor_time_) * 0.001;
  last_sensor_time_ = mcs_time;

  std::unique_lock<std::mutex> vl(velo_lock_);
  double right_da = (to_right - right_velo_) * ims;
  double top_da = (to_top - top_velo_) * ims;
  double roll_da = (to_clockwork - clock_velo_) * ims;
  vl.unlock();

  std::unique_lock<std::mutex> lk(helm_axis_lock);
  bool cv = center_view_flag_.exchange(false);
  if (cv) {
    helm_forward = vec3d(0.0, 0.0, 1.0);
    helm_up = vec3d(0.0, 1.0, 0.0);
  } else {
    RotateAxis(helm_forward, helm_up, right_da, top_da, roll_da);
    auto pv = glm::perp(helm_up, helm_forward);
    helm_up = pv;
  }
  lk.unlock();


#ifdef DEBUG_HELMET_VIEW
  // Отладочный вывод позиции шлема раз в секунду
  static auto mark = std::chrono::steady_clock::now() + std::chrono::seconds(1);
  static size_t counter = 0;
  ++counter;
  auto curt = std::chrono::steady_clock::now();
  if (curt >= mark) {
    mark = curt + std::chrono::seconds(1);

    double ra, ta, ca;
    GetViewPoint(ra, ta, ca);
    std::cout << "Helm angles: right=" << int(ra * 180.0 / kPi)
              << ", top=" << int(ta * 180.0 / kPi)
              << ", clockrotation=" << int(ca * 180.0 / kPi)
              << ". Points per second=" << counter << std::endl;
    counter = 0;
  }
#endif  // DEBUG_HELMET_VIEW
}


void PsvrHelmetView::RotateAxis(PsvrHelmetView::vec3d& forward,
    PsvrHelmetView::vec3d& up, double right_angle, double top_angle,
    double clock_angle) {
  auto right = -glm::cross(forward, up);
  auto fv1 = glm::rotate(forward, glm::radians(right_angle), up);
  auto rv1 = glm::rotate(right, glm::radians(right_angle), up);
  auto fv2 = glm::rotate(fv1, glm::radians(-top_angle), right);
  auto uv1 = glm::rotate(up, glm::radians(-top_angle), right);
  auto rv2 = glm::rotate(rv1, glm::radians(-clock_angle), forward);
  auto uv2 = glm::rotate(uv1, glm::radians(-clock_angle), forward);

  forward = glm::normalize(fv2);
  right = glm::normalize(rv2);
  up = glm::normalize(uv2);
}


PsvrHelmetView::~PsvrHelmetView() {}


void PsvrHelmetView::SetVRMode(IHelmet::VRMode mode) {
  SplitScreen(mode == IHelmet::VRMode::kSplitScreen);
}


void PsvrHelmetView::CenterView() { center_view_flag_ = true; }

void PsvrHelmetView::GetViewPoint(
    double& right_angle, double& top_angle, double& clock_angle) {
  std::unique_lock<std::mutex> lk(helm_axis_lock);
  bool cv = center_view_flag_.exchange(false);
  if (cv) {
    helm_forward = vec3d(0.0, 0.0, 1.0);
    helm_up = vec3d(0.0, 1.0, 0.0);
    right_angle = 0.0;
    top_angle = 0.0;
    clock_angle = 0.0;
    return;
  }

  right_angle = atan2(helm_forward.x, helm_forward.z);
  if (helm_forward.y >= 1.0) {
    top_angle = kPi / 2.0;
  } else if (helm_forward.y <= -1.0) {
    top_angle = -kPi / 2.0;
  } else {
    top_angle = asin(helm_forward.y);
  }

  auto horz = glm::cross(helm_forward, vec3d(0.0, 1.0, 0.0));
  if (glm::length2(horz) < kNearZeroLength2) {
    clock_angle = 0.0;
  } else {
    clock_angle = kPi / 2.0 - glm::angle(glm::normalize(horz), helm_up);
  }
}
