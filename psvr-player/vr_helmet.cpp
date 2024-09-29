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
#include <stdexcept>
#include <thread>

#include <hidapi/hidapi.h>


class PSVRHelmet: public IHelmet {
 public:
  PSVRHelmet();
  virtual ~PSVRHelmet();

  bool IsOpened();

  void SetVRMode(VRMode mode) override;


 private:
  PSVRHelmet(const PSVRHelmet&) = delete;
  PSVRHelmet(PSVRHelmet&&) = delete;
  PSVRHelmet& operator=(const PSVRHelmet&) = delete;
  PSVRHelmet& operator=(PSVRHelmet&&) = delete;

  const unsigned short kPsvrVendorID = 0x054c;
  const unsigned short kPsvrProductID = 0x09af;
  const char kPSVRHelmetInterface[4] = ":05";
  static const size_t kMaxBufferSize = 128;

  unsigned char buffer_[kMaxBufferSize];
  void* device_;  //!< Opened control device with hid_device* type. Or nullptr

  std::thread read_thread_;  //!< Поток чтения позиции шлема
  std::atomic_bool shutdown_flag_;  //!< Флаг завершения поток чтения

  bool OpenDevice();
  void CloseDevice();
  bool SplitScreen(bool split_mode);

  std::string GetControlDevice();

  void ReadHid();
};

std::shared_ptr<IHelmet> CreateHelmet() {
  try {
    return std::shared_ptr<PSVRHelmet>(new PSVRHelmet);
  } catch (...) {
  }
  return std::shared_ptr<IHelmet>();
}


PSVRHelmet::PSVRHelmet(): device_(nullptr) {
  shutdown_flag_ = false;
  if (!OpenDevice()) {
    throw std::runtime_error("Can't open hid device");
  }

  std::thread t([this]() { ReadHid(); });
  std::swap(read_thread_, t);
  assert(!t.joinable());
}


std::string PSVRHelmet::GetControlDevice() {
  auto devs = hid_enumerate(kPsvrVendorID, kPsvrProductID);
  if (!devs) {
    return std::string();
  }

  std::string res;
  for (auto dev = devs; dev; dev = dev->next) {
    try {
      std::string p = dev->path;
      if (p.substr(p.length() - 3) == kPSVRHelmetInterface) {
        res = p;
        break;
      }
    } catch (std::bad_alloc&) {
    } catch (std::out_of_range&) {
    }
  }

  hid_free_enumeration(devs);

  return res;
}

void PSVRHelmet::ReadHid() {
  while (true) {
    if (shutdown_flag_.load(std::memory_order_acquire)) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
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
  assert(!device_);

  auto dev_name = GetControlDevice();
  if (dev_name.empty()) {
    return false;
  }

  device_ = hid_open_path(dev_name.c_str());
  if (!device_) {
    return false;
  }

  return true;
}

void PSVRHelmet::CloseDevice() {
  if (device_) {
    SplitScreen(false);
    hid_close((hid_device*)device_);
    device_ = nullptr;
  }
}

bool PSVRHelmet::IsOpened() { return device_; }

void PSVRHelmet::SetVRMode(IHelmet::VRMode mode) {
  SplitScreen(mode == IHelmet::VRMode::kSplitScreen);
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

  return hid_write((hid_device*)device_, buffer_, 8) != -1;
}
