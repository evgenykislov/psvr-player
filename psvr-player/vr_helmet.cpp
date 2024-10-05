/*
 * Created by Evgeny Kislov <dev@evgenykislov.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include "vr_helmet.h"

#include <atomic>
#include <cassert>
#include <fstream>
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


class PSVRHelmet: public IHelmet {
 public:
  PSVRHelmet();
  virtual ~PSVRHelmet();

  bool IsOpened();

  void SetVRMode(VRMode mode) override;
  void CenterView() override;
  void GetViewPoint(
      double& right_angle, double& top_angle, double& clock_angle) override;

 private:
  PSVRHelmet(const PSVRHelmet&) = delete;
  PSVRHelmet(PSVRHelmet&&) = delete;
  PSVRHelmet& operator=(const PSVRHelmet&) = delete;
  PSVRHelmet& operator=(PSVRHelmet&&) = delete;

  using vec3d = glm::vec<3, double, glm::defaultp>;

  const unsigned short kPsvrVendorID = 0x054c;
  const unsigned short kPsvrProductID = 0x09af;
  const char kPSVRControlInterface[4] = ":05";
  const char kPSVRSensorsInterface[4] = ":04";
  static const size_t kMaxBufferSize = 128;
  static const int kReadTimeout =
      10;  //!< Таймаут на чтение данных из hid-устройства
  const double kAccelerationScale = 0.00003125;
  const double kNearZeroLength2 =
      1.0e-8;  //!< Длина очень короткого вектора. При расчётах углов означает
               //!< взгляд ровно вверх или вниз


  unsigned char buffer_[kMaxBufferSize];
  std::atomic<void*>
      device_1;  //!< Opened control device with hid_device* type. Or nullptr
  std::atomic<void*> sensors_;  //!< Устройство-сенсоры шлема

  std::thread read_thread_;  //!< Поток чтения позиции шлема
  std::atomic_bool shutdown_flag_;  //!< Флаг завершения поток чтения
  std::atomic_bool center_view_flag_;

  vec3d helm_forward =
      vec3d(0.0, 0.0, 1.0);  //!< Вектор указывает куда смотрит (вперёд) шлем в
                             //!< мировых координатах
  vec3d helm_up = vec3d(0.0, 1.0,
      0.0);  //!< Вектор указывает куда смотрит верх шлема в мировых координатах
  vec3d helm_right =
      vec3d(1.0, 0.0, 0.0);  //!< Вектор указывает куда смотрит правая сторона
                             //!< шлема в мировых координатах
  std::mutex helm_axis_lock;

  bool OpenDevice();
  void CloseDevice();
  bool SplitScreen(bool split_mode);

  bool GetHidNames(std::string& control, std::string& sensors);

  void ReadHid();

  static int16_t read_int16(const unsigned char* buffer, int offset);

  /*! Есть три направления в мировых координатах: вперёд, направо и вверх
  относительно шлема Эту систему координат нужно повернуть на заданные углы
  относительно самих себя */
  void RotateAxis(vec3d& forward, vec3d& right, vec3d& up, double right_angle,
      double top_angle, double clock_angle);
};

std::shared_ptr<IHelmet> CreateHelmet() {
  try {
    return std::shared_ptr<PSVRHelmet>(new PSVRHelmet);
  } catch (...) {
  }
  return std::shared_ptr<IHelmet>();
}


PSVRHelmet::PSVRHelmet(): device_1(nullptr), sensors_(nullptr) {
  shutdown_flag_ = false;
  center_view_flag_ = true;
  if (!OpenDevice()) {
    throw std::runtime_error("Can't open hid device");
  }

  std::thread t([this]() { ReadHid(); });
  std::swap(read_thread_, t);
  assert(!t.joinable());
}


bool PSVRHelmet::GetHidNames(std::string& control, std::string& sensors) {
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

void PSVRHelmet::ReadHid() {
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
      helm_right = vec3d(1.0, 0.0, 0.0);
    } else {
      RotateAxis(helm_forward, helm_right, helm_up, right_da, top_da, roll_da);
      auto pv = glm::perp(helm_up, helm_forward);
      helm_up = pv;
    }
    lk.unlock();


#ifdef DEBUG_HELMET_VIEW
    // Отладочный вывод позиции шлема раз в секунду
    static auto mark =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    auto curt = std::chrono::steady_clock::now();
    if (curt >= mark) {
      mark = curt + std::chrono::seconds(1);

      double ra, ta, ca;
      GetViewPoint(ra, ta, ca);
      std::cout << "Helm angles: right=" << int(ra * 180.0 / kPi)
                << ", top=" << int(ta * 180.0 / kPi)
                << ", clockrotation=" << int(ca * 180.0 / kPi) << std::endl;
    }
#endif  // DEBUG_HELMET_VIEW
  }
}


int16_t PSVRHelmet::read_int16(const unsigned char* buffer, int offset) {
  int16_t v;
  v = buffer[offset];
  v |= buffer[offset + 1] << 8;
  return v;
}

void PSVRHelmet::RotateAxis(PSVRHelmet::vec3d& forward,
    PSVRHelmet::vec3d& right, PSVRHelmet::vec3d& up, double right_angle,
    double top_angle, double clock_angle) {
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


PSVRHelmet::~PSVRHelmet() {
  shutdown_flag_.store(true, std::memory_order_release);
  if (read_thread_.joinable()) {
    read_thread_.join();
  }

  CloseDevice();
}

bool PSVRHelmet::OpenDevice() {
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

void PSVRHelmet::CloseDevice() {
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

bool PSVRHelmet::IsOpened() { return device_1 && sensors_; }

void PSVRHelmet::SetVRMode(IHelmet::VRMode mode) {
  SplitScreen(mode == IHelmet::VRMode::kSplitScreen);
}


void PSVRHelmet::CenterView() { center_view_flag_ = true; }

void PSVRHelmet::GetViewPoint(
    double& right_angle, double& top_angle, double& clock_angle) {
  std::unique_lock<std::mutex> lk(helm_axis_lock);
  bool cv = center_view_flag_.exchange(false);
  if (cv) {
    right_angle = 0.0;
    top_angle = 0.0;
    clock_angle = 0.0;
    return;
  }

  std::cout << "Vectors angles: "
            << int(glm::angle(helm_right, helm_up) * 180.0 / kPi) << ", "
            << int(glm::angle(helm_right, helm_forward) * 180.0 / kPi) << ", "
            << int(glm::angle(helm_forward, helm_up) * 180.0 / kPi)
            << std::endl;

  right_angle = atan2(helm_forward.x, helm_forward.z);
  top_angle = atan2(helm_forward.y, helm_forward.z);

  auto horz = glm::cross(helm_forward, vec3d(0.0, 1.0, 0.0));
  if (glm::length2(horz) < kNearZeroLength2) {
    clock_angle = 0.0;
  } else {
    clock_angle = kPi / 2.0 - glm::angle(glm::normalize(horz), helm_up);
  }
}


bool PSVRHelmet::SplitScreen(bool split_mode) {
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
