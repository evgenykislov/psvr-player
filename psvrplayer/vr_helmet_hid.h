#ifndef PSVRHELMETHID_H
#define PSVRHELMETHID_H

#include <atomic>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

class libusb_context;

/*!
 * @brief Описание интерфейса-конечной точки для работы со шлемом
 */
struct PointDescription {
  unsigned int Interface;
  unsigned int AltSetting;
  unsigned int Endpoint;
  std::string Description;
  bool ControlPoint;

  std::string FormatName() {
    std::stringstream r;
    r << Description << " {" << Interface << "-" << AltSetting << "-" << Endpoint << "}";
    return r.str();
  }

  uint32_t Serialize() {
    uint32_t r = 0;
    r |= Interface & 0xff;
    r <<= 8;
    r |= AltSetting & 0xff;
    r <<= 8;
    r |= Endpoint;
    return r;
  }

  void Deserialize(uint32_t v) {
    Endpoint = v & 0xff;
    AltSetting = (v >> 8) & 0xff;
    Interface = (v >> 16) & 0xff;
    ControlPoint = false;
    Description.clear();
  }

};

/*! Класс для обработки hid-устройств vr-шлема psvr */
class PsvrHelmetHid {
 public:
  PsvrHelmetHid();
  virtual ~PsvrHelmetHid();

  /*! Получить список всех устройств, относящихся к шлему PSVR
  \return список точек для подключения с именами. Если список пустой, то устройства нет или занято */
  static std::vector<PointDescription> GetDevicesName();

 protected:
  /*! Функция для обработки данных скорости поворота от сенсоров. Скорость
  измеряется в градусах в миллисекунду. Функция вызывается в отдельном потоке.
  Функция не должна задерживать выполнение потока (обычный интервал прихода
  следующих данных - 1 мс) \param to_right_accel скорость поворота вправо \param
  to_top_accel скорость поворота вверх \param to_clockwork_accel скорость
  поворота по часовой стрелке
  \param mcs_time глобальный счётчик времени (всегда увеличивается) */
  virtual void OnSensorsData(
      double to_right, double to_top, double to_clockwork, uint64_t mcs_time){};

  bool SplitScreen(bool split_mode);

 private:
  PsvrHelmetHid(const PsvrHelmetHid&) = delete;
  PsvrHelmetHid(PsvrHelmetHid&&) = delete;
  PsvrHelmetHid& operator=(const PsvrHelmetHid&) = delete;
  PsvrHelmetHid& operator=(PsvrHelmetHid&&) = delete;

  static const size_t kMaxBufferSize = 128;
  static const int kReadTimeout =
      10;  //!< Таймаут на чтение данных из hid-устройства
  const double kAccelerationScale = 0.00003125;

  static const unsigned short kPsvrVendorID = 0x054c;
  static const unsigned short kPsvrProductID = 0x09af;

  libusb_context* usb_context_;

  std::atomic<void*>
      control_;  //!< Opened control device with hid_device* type. Or nullptr
  std::atomic<void*> sensors_;  //!< Устройство-сенсоры шлема
  uint64_t sensor_timer_;  //!< Часы таймера. Монотонно растут, отсчитывают
                           //!< микросекунды
  std::thread read_thread_;  //!< Поток чтения позиции шлема
  std::atomic_bool shutdown_flag_;  //!< Флаг завершения поток чтения

  unsigned char buffer_[kMaxBufferSize];

  bool OpenDevice();
  void CloseDevice();

  /*! Функция вычитывания состояния сенсоров. Обычно выполняется в отдельном
  потоке. Завершается при выставлении флага shutdown_flag_ */
  void ReadHid();

  /*! Функция вычитывания из буфера 16-битного значения
  \param buffer буфер с данными
  \param offset смещение числа
  \return число из буфера */
  static int16_t read_int16(const unsigned char* buffer, int offset);

  static int32_t read_int32(const unsigned char* buffer, int offset);
};

#endif  // PSVRHELMETHID_H
