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


class PsvrHelmetView: public IHelmet, protected PsvrHelmetHid {
 public:
  PsvrHelmetView();
  virtual ~PsvrHelmetView();

  bool IsOpened();

  void SetVRMode(VRMode mode) override;
  void CenterView() override;
  void GetViewPoint(
      double& right_angle, double& top_angle, double& clock_angle) override;

 protected:
  virtual void OnSensorsData(double to_right, double to_top,
      double to_clockwork, uint64_t mcs_time) override;

 private:
  PsvrHelmetView(const PsvrHelmetView&) = delete;
  PsvrHelmetView(PsvrHelmetView&&) = delete;
  PsvrHelmetView& operator=(const PsvrHelmetView&) = delete;
  PsvrHelmetView& operator=(PsvrHelmetView&&) = delete;

  using vec3d = glm::vec<3, double, glm::defaultp>;

  const double kNearZeroLength2 =
      1.0e-8;  //!< Длина очень короткого вектора. При расчётах углов означает
               //!< взгляд ровно вверх или вниз


  std::atomic_bool center_view_flag_;
  uint64_t last_sensor_time_;

  vec3d helm_forward =
      vec3d(0.0, 0.0, 1.0);  //!< Вектор указывает куда смотрит (вперёд) шлем в
                             //!< мировых координатах
  vec3d helm_up = vec3d(0.0, 1.0,
      0.0);  //!< Вектор указывает куда смотрит верх шлема в мировых координатах
  std::mutex helm_axis_lock;

  /*! Есть три направления в мировых координатах: вперёд, направо и вверх
  относительно шлема Эту систему координат нужно повернуть на заданные углы
  относительно самих себя */
  void RotateAxis(vec3d& forward, vec3d& up, double right_angle,
      double top_angle, double clock_angle);
};


#endif  // PSVRHELMETVIEW_H
