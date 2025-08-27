#ifndef PSVRHELMETVIEW_H
#define PSVRHELMETVIEW_H

#include <atomic>
#include <mutex>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "rotation.h"
#include "vr_helmet.h"
#include "vr_helmet_hid.h"


class PsvrHelmetView: public IHelmet, protected PsvrHelmetHid {
 public:
  PsvrHelmetView();
  virtual ~PsvrHelmetView();

  bool IsOpened();

  void SetVRMode(VRMode mode) override;
  void CenterView() override;
  void GetViewPoint(glm::mat4& rotation) override;
  void SetRotationSpeedup(double speedup) override;

 protected:
  virtual void OnSensorsData(double to_right, double to_top,
      double to_clockwork, uint64_t mcs_time) override;

 private:
  PsvrHelmetView(const PsvrHelmetView&) = delete;
  PsvrHelmetView(PsvrHelmetView&&) = delete;
  PsvrHelmetView& operator=(const PsvrHelmetView&) = delete;
  PsvrHelmetView& operator=(PsvrHelmetView&&) = delete;


  const int64_t kFixedPointFactor = 1000000000L;

  using vec3d = glm::vec<3, double, glm::defaultp>;

  std::atomic_bool center_view_flag_;
  uint64_t last_sensor_time_;

  double right_velo_;  //!< Скорость "дрейфа" шлема вправо (из калибровки)
  double top_velo_;  //!< Скорость "дрейфа" шлема вверх (из калибровки)
  double clock_velo_;  //!< Скорость "дрейфа" шлема по часовой стрелке (из
                       //!< калибровки)
  std::mutex velo_lock_;

  Rotation rotation_;  //!< Математика для расчёта вращений
};


#endif  // PSVRHELMETVIEW_H
