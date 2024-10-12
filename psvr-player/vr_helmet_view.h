#ifndef PSVRHELMETVIEW_H
#define PSVRHELMETVIEW_H

#include <atomic>
#include <mutex>
#include <thread>

#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "vr_helmet.h"
#include "vr_helmet_hid.h"


class PsvrHelmetView: public IHelmet {
 public:
  PsvrHelmetView();
  virtual ~PsvrHelmetView();

  bool IsOpened();

  void SetVRMode(VRMode mode) override;
  void CenterView() override;
  void GetViewPoint(
      double& right_angle, double& top_angle, double& clock_angle) override;

 private:
  PsvrHelmetView(const PsvrHelmetView&) = delete;
  PsvrHelmetView(PsvrHelmetView&&) = delete;
  PsvrHelmetView& operator=(const PsvrHelmetView&) = delete;
  PsvrHelmetView& operator=(PsvrHelmetView&&) = delete;

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
  void RotateAxis(vec3d& forward, vec3d& up, double right_angle,
      double top_angle, double clock_angle);
};


#endif  // PSVRHELMETVIEW_H
