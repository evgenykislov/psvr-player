#include "vr_helmet_view.h"

#include <cassert>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <hidapi.h>

#include "iniparser.h"
#include "home-dir.h"

const char kConfigFileName[] = "/psvrplayer.cfg";

PsvrHelmetView::PsvrHelmetView() {
  center_view_flag_ = true;
  last_sensor_time_ = std::numeric_limits<uint64_t>::max();

  auto cfg = HomeDirLibrary::GetDataDir() + kConfigFileName;
  std::unique_lock<std::mutex> vl(velo_lock_);
  auto dict = iniparser_load(cfg.c_str());
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

  bool cv = center_view_flag_.exchange(false);
  if (cv) {
    rotation_.Reset();
  } else {
    rotation_.Rotate(right_da, top_da, roll_da);
  }
}


PsvrHelmetView::~PsvrHelmetView() {}


void PsvrHelmetView::SetVRMode(IHelmet::VRMode mode) {
  SplitScreen(mode == IHelmet::VRMode::kSplitScreen);
}


void PsvrHelmetView::CenterView() { center_view_flag_ = true; }

void PsvrHelmetView::GetViewPoint(glm::mat4& rot_mat) {
  bool cv = center_view_flag_.exchange(false);
  if (cv) {
    rotation_.Reset();
  }

  rotation_.GetSummRotation(rot_mat);
}

void PsvrHelmetView::SetRotationSpeedup(double speedup) {
  rotation_.SetRotationSpeedup(speedup);
}
