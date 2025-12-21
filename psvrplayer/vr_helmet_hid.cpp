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

PsvrHelmetHid::PsvrHelmetHid()
    : control_(uint32_t(-1)),
      sensors_(uint32_t(-1)),
      usb_context_(nullptr),
      usb_device_(nullptr) {
  shutdown_flag_ = false;
  sensor_timer_ = 0;

  auto ur = libusb_init_context(&usb_context_, nullptr, 0);
  if (ur != 0) {
    throw std::runtime_error("Can't open usb support");
  }

//  libusb_set_option(
//      usb_context_, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

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
          point.Endpoint = ed->bEndpointAddress;
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

  uint32_t control;
  uint32_t sensors;

  Config::GetDevicesName(&control, &sensors);

  if (control == uint32_t(-1) || sensors == uint32_t(-1)) {
    std::wcerr << "Names for control and sensors aren't specified" << std::endl;
    return false;
  }

  control_ = control;
  sensors_ = sensors;

  usb_device_ = libusb_open_device_with_vid_pid(usb_context_, kPsvrVendorID, kPsvrProductID);
  if (!usb_device_) {
    std::wcerr << "Can't open vr helmet device" << std::endl;
    return false;
  }

  return true;
}


void PsvrHelmetHid::CloseDevice() {
  if (usb_device_) {
    SplitScreen(false); // Принудительно возвращаем обычный режим (неразделённый)
    libusb_close(usb_device_);
    usb_device_ = nullptr;
  }
}


void PsvrHelmetHid::ReadHid() {
  assert(usb_device_);

  PointDescription snr;
  snr.Deserialize(sensors_.load());

  int err = libusb_claim_interface(usb_device_, snr.Interface);
  if (err != 0) {
    std::wcerr << "Error of device sensors claiming: " << libusb_strerror(err) << std::endl;
    return;
  }

  // TODO Использовать аппаратные часы таймера вместо компьютерных
  std::chrono::steady_clock::time_point prev_reading =
      std::chrono::steady_clock::now();

  while (true) {
    if (shutdown_flag_.load(std::memory_order_acquire)) {
      break;
    }

    const int kPacketSize = 64;
    const int kMaxBufferSize = 70;
    unsigned char buffer[kMaxBufferSize];
    int transf;
    int res = libusb_interrupt_transfer(usb_device_, snr.Endpoint, buffer,
        kMaxBufferSize, &transf, kReadTimeout);
    auto ct = std::chrono::steady_clock::now();

    if (res != 0) {
      // Ошибка чтения устройства
      if (res == LIBUSB_ERROR_TIMEOUT) {
        // Недождались. Подождём ещё
        continue;
      }
      
      // Остальное, похоже, не лечится
      std::wcerr << "Error of device sensors reading: " << libusb_strerror(res)
                 << std::endl;
      break;
    }

    if ((transf != kPacketSize) && (transf != kPacketSize + 1)) {
      // Пришли данные неожиданного размера
      // Прим.: может передаваться завершающий 0 вне запрашиваемого пакета
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

  err = libusb_release_interface(usb_device_, snr.Interface);
  if (err != 0) {
    std::wcerr << "Error of device sensors releasing: " << libusb_strerror(err)
               << std::endl;
    return;
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
  bool r = true;
  assert(usb_device_);

  PointDescription cnt;
  cnt.Deserialize(control_.load());

  int err = libusb_claim_interface(usb_device_, cnt.Interface);
  if (err != 0) {
    std::wcerr << "Error of device control claiming: " << libusb_strerror(err)
               << std::endl;
    return false;
  }

  const size_t kBufferSize = 8;
  unsigned char buffer[kBufferSize];
  buffer[0] = 0x23;
  buffer[1] = 0x00;
  buffer[2] = 0xaa;
  buffer[3] = 0x04;
  buffer[4] = split_mode ? 0x01 : 0x00;
  buffer[5] = 0x00;
  buffer[6] = 0x00;
  buffer[7] = 0x00;

  int transf;
  int res = libusb_interrupt_transfer(
      usb_device_, cnt.Endpoint, buffer, kBufferSize, &transf, kWriteTimeout);

  if (res != 0) {
    // Ошибка записи устройства
    std::wcerr << "Error of device control writing: " << libusb_strerror(res)
                << std::endl;
    r = false;
  } else if (transf != kBufferSize) {
    // Записались не все данные
    std::wcerr << "Can't write whole packet of data" << std::endl;
    r = false;
  }

  err = libusb_release_interface(usb_device_, cnt.Interface);
  if (err != 0) {
    std::wcerr << "Error of device releasing releasing: " << libusb_strerror(err)
               << std::endl;
    return false;
  }

  return r;
}
