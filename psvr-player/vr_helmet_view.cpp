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

#define DEBUG_HELMET_VIEW

const double kPi = 3.1415926535897932384626433832795;

PsvrHelmetView::PsvrHelmetView(): device_1(nullptr), sensors_(nullptr) {
  shutdown_flag_ = false;
  center_view_flag_ = true;
  if (!OpenDevice()) {
    throw std::runtime_error("Can't open hid device");
  }

  std::thread t([this]() { ReadHid(); });
  std::swap(read_thread_, t);
  assert(!t.joinable());
}


bool PsvrHelmetView::GetHidNames(std::string& control, std::string& sensors) {
  auto devs = hid_enumerate(kPsvrVendorID, kPsvrProductID);
  if (!devs) {
    return false;
  }

  control.clear();
  sensors.clear();
  for (auto dev = devs; dev; dev = dev->next) {
    try {
      std::string p = dev->path;
      std::string tail = p.substr(p.length() - 3);
      if (tail == kPSVRControlInterface) {
        control = p;
      }
      if (tail == kPSVRSensorsInterface) {
        sensors = p;
      }
    } catch (std::bad_alloc&) {
    } catch (std::out_of_range&) {
    }
  }

  hid_free_enumeration(devs);

  return true;
}

void PsvrHelmetView::ReadHid() {
  hid_device* dev = (hid_device*)sensors_.load();
  if (!dev) {
    return;
  }

  std::chrono::steady_clock::time_point prev_reading =
      std::chrono::steady_clock::now();

  while (true) {
    if (shutdown_flag_.load(std::memory_order_acquire)) {
      break;
    }

    const int kPacketSize = 64;
    unsigned char buffer[kMaxBufferSize];
    int size = hid_read_timeout(dev, buffer, kMaxBufferSize, kReadTimeout);
    auto ct = std::chrono::steady_clock::now();

    if (size == -1) {
      // Ошибка чтения устройства. Похоже, это не лечится
      break;
    }
    if (size == 0) {
      // Ждали. Данных нет
      continue;
    }

    if (size != kPacketSize) {
      continue;
    }

    int16_t right_acc = read_int16(buffer, 20) + read_int16(buffer, 36);
    int16_t top_acc = read_int16(buffer, 22) + read_int16(buffer, 38);
    int16_t roll_acc = read_int16(buffer, 24) + read_int16(buffer, 40);

    double ims =
        std::chrono::duration_cast<std::chrono::microseconds>(ct - prev_reading)
            .count() *
        0.001;
    prev_reading = ct;

    // TODO Make calibration
    double right_velo_ = -0.00008;
    double top_velo_ = -0.000645;
    double roll_velo_ = +0.00021;

    double right_da = -(right_acc * kAccelerationScale - right_velo_) * ims;
    double top_da = (top_acc * kAccelerationScale - top_velo_) * ims;
    double roll_da = -(roll_acc * kAccelerationScale - roll_velo_) * ims;

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
    static auto mark =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
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
}


int16_t PsvrHelmetView::read_int16(const unsigned char* buffer, int offset) {
  int16_t v;
  v = buffer[offset];
  v |= buffer[offset + 1] << 8;
  return v;
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


PsvrHelmetView::~PsvrHelmetView() {
  shutdown_flag_.store(true, std::memory_order_release);
  if (read_thread_.joinable()) {
    read_thread_.join();
  }

  CloseDevice();
}

bool PsvrHelmetView::OpenDevice() {
  CloseDevice();
  assert(!device_1);
  assert(!sensors_);

  std::string control;
  std::string sensors;

  if (!GetHidNames(control, sensors)) {
    return false;
  }

  device_1 = hid_open_path(control.c_str());
  if (!device_1) {
    return false;
  }

  sensors_ = hid_open_path(sensors.c_str());
  if (!sensors_) {
    hid_close((hid_device*)device_1.load());
    device_1 = nullptr;
    return false;
  }

  return true;
}

void PsvrHelmetView::CloseDevice() {
  void* d = nullptr;
  device_1.exchange(d);
  if (d) {
    SplitScreen(false);
    hid_close((hid_device*)d);
    d = nullptr;
  }

  sensors_.exchange(d);
  if (d) {
    hid_close((hid_device*)d);
    d = nullptr;
  }
}

bool PsvrHelmetView::IsOpened() { return device_1 && sensors_; }

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


bool PsvrHelmetView::SplitScreen(bool split_mode) {
  buffer_[0] = 0x23;
  buffer_[1] = 0x00;
  buffer_[2] = 0xaa;
  buffer_[3] = 0x04;
  buffer_[4] = split_mode ? 0x01 : 0x00;
  buffer_[5] = 0x00;
  buffer_[6] = 0x00;
  buffer_[7] = 0x00;

  return hid_write((hid_device*)device_1.load(), buffer_, 8) != -1;
}
