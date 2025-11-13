#include "vr_helmet_hid.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <hidapi.h>
#include <libusb.h>

#include "config_file.h"

// Хак для обработки хидеров: товарисчи везде пихают свою реализацию min
#undef min

PsvrHelmetHid::PsvrHelmetHid(): control_(nullptr), sensors_(nullptr) {
  shutdown_flag_ = false;
  sensor_timer_ = 0;

  auto ur = libusb_init_context(&usb_context_, nullptr, 0);
  if (ur != 0) {
    throw std::runtime_error("Can't open usb support");
  }

  if (!OpenDevice()) {
    libusb_exit(usb_context_);
    usb_context_ = nullptr;
    throw std::runtime_error("Can't open hid device");
  }

  std::thread t([this]() { ReadHid(); });
  std::swap(read_thread_, t);
  assert(!t.joinable());
}


PsvrHelmetHid::~PsvrHelmetHid() {
  shutdown_flag_.store(true, std::memory_order_release);
  if (read_thread_.joinable()) {
    read_thread_.join();
  }

  CloseDevice();

  libusb_exit(usb_context_);
  usb_context_ = nullptr;
}


std::vector<PointDescription> PsvrHelmetHid::GetDevicesName() {
  std::vector<PointDescription> res;

  // Статичная функция с персональной инициализацией всего
  libusb_context* ctx;
  if (libusb_init_context(&ctx, nullptr, 0) != 0) {
    std::wcerr << "Failed initialize usb subsystem" << std::endl;
    return {};
  }
  auto h = libusb_open_device_with_vid_pid(ctx, kPsvrVendorID, kPsvrProductID);
  if (!h) {
    libusb_exit(ctx);
    return res;
  }


  libusb_device* dev = libusb_get_device(h);
  libusb_config_descriptor* cfg;
  const int kBufferLen = 200;
  unsigned char buffer[kBufferLen];
  PointDescription point;
  if (libusb_get_active_config_descriptor(dev, &cfg) >= 0) {
    for (int i = 0; i < cfg->bNumInterfaces; ++i) {
      const libusb_interface* iff = &(cfg->interface[i]);
      for (int j = 0; j < iff->num_altsetting; ++j) {        
        const struct libusb_interface_descriptor* id = &(iff->altsetting[j]);
        point.Interface = id->bInterfaceNumber;
        point.AltSetting = id->bAlternateSetting;
        point.Description.clear();
        // Имя интерфейса
        int l = libusb_get_string_descriptor_ascii(
            h, id->iInterface, buffer, kBufferLen);
        if (l >= 0 && l < kBufferLen) {
            point.Description = std::string(buffer, buffer + l);
        }

        // Поищём точки ввода и вывода для передачи по прерываниям
        bool has_in_intr = false;
        bool has_out_intr = false;
        for (int e = 0; e < id->bNumEndpoints; ++e) {
          const struct libusb_endpoint_descriptor* ed = &(id->endpoint[e]);
          if ((ed->bmAttributes & 0x03) != LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT) {
            continue;
          }
          point.Endpoint = ed->bEndpointAddress & 0x0f;
          point.ControlPoint = (ed->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_OUT;
          res.push_back(point);
        }
      }
    }

    libusb_free_config_descriptor(cfg);
  }

  libusb_close(h);

  libusb_exit(ctx);

  return res;
}


bool PsvrHelmetHid::OpenDevice() {
  CloseDevice();
  assert(!control_);
  assert(!sensors_);

  uint32_t control;
  uint32_t sensors;

  Config::GetDevicesName(&control, &sensors);

  if (control == uint32_t(-1) || sensors == uint32_t(-1)) {
    std::wcerr << "Names for control and sensors aren't specified" << std::endl;
    return false;
  }

  return false;
/*
  control_ = hid_open_path(control.c_str());
  if (!control_) {
    auto he = hid_error(nullptr);
    if (he) {
      std::wcerr << "Helmet control open error: " << he << std::endl;
    }
    return false;
  }

  sensors_ = hid_open_path(sensors.c_str());
  if (!sensors_) {
    auto he = hid_error(nullptr);
    if (he) {
      std::wcerr << "Helmet sensors open error: " << he << std::endl;
    }
    hid_close((hid_device*)control_.load());
    control_ = nullptr;
    return false;
  }
*/
  return true;
}


void PsvrHelmetHid::CloseDevice() {
  void* d = nullptr;
  control_.exchange(d);
  if (d) {
    SplitScreen(false);
    hid_close((hid_device*)d);
    auto he = hid_error(nullptr);
    if (he) {
      std::wcerr << "Helmet control close error: " << he << std::endl;
    }
  }

  d = nullptr;
  sensors_.exchange(d);
  if (d) {
    hid_close((hid_device*)d);
    auto he = hid_error(nullptr);
    if (he) {
      std::wcerr << "Helmet sensors close error: " << he << std::endl;
    }
  }
}


void PsvrHelmetHid::ReadHid() {
  hid_device* dev = (hid_device*)sensors_.load();
  if (!dev) {
    return;
  }

  // TODO Использовать аппакратные часы таймера вместо компьютерных
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

    static int32_t last_st = std::numeric_limits<int32_t>::min();
    auto st = read_int32(buffer, 16);
    if (last_st == std::numeric_limits<int32_t>::min()) {
      last_st = st;
    }
    auto dst = st - last_st;  // TODO Use hardware timer for calculation
    last_st = st;

    int64_t ims =
        std::chrono::duration_cast<std::chrono::microseconds>(ct - prev_reading)
            .count();
    prev_reading = ct;
    sensor_timer_ += ims;

    double right_da = -(right_acc * kAccelerationScale);
    double top_da = (top_acc * kAccelerationScale);
    double roll_da = -(roll_acc * kAccelerationScale);

    OnSensorsData(right_da, top_da, roll_da, sensor_timer_);
  }
}


int16_t PsvrHelmetHid::read_int16(const unsigned char* buffer, int offset) {
  int16_t v;
  v = buffer[offset];
  v |= buffer[offset + 1] << 8;
  return v;
}


int32_t PsvrHelmetHid::read_int32(const unsigned char* buffer, int offset) {
  return (buffer[offset + 0] << 0) | (buffer[offset + 1] << 8) |
         (buffer[offset + 2] << 16) | (buffer[offset + 3] << 24);
}


bool PsvrHelmetHid::SplitScreen(bool split_mode) {
  buffer_[0] = 0x23;
  buffer_[1] = 0x00;
  buffer_[2] = 0xaa;
  buffer_[3] = 0x04;
  buffer_[4] = split_mode ? 0x01 : 0x00;
  buffer_[5] = 0x00;
  buffer_[6] = 0x00;
  buffer_[7] = 0x00;

  return hid_write((hid_device*)control_.load(), buffer_, 8) != -1;
}
