#ifndef PSVRHELMETCALIBRATION_H
#define PSVRHELMETCALIBRATION_H

#include <mutex>

#include "vr_helmet.h"
#include "vr_helmet_hid.h"


class PsvrHelmetCalibration: public IHelmet, PsvrHelmetHid {
 public:
  PsvrHelmetCalibration();
  ~PsvrHelmetCalibration();

  // Virtual functions (not used here)
  void SetVRMode(VRMode mode) override{};
  void CenterView() override{};
  void GetViewPoint(double&, double&, double&) override{};

 protected:
  virtual void OnSensorsData(double to_right, double to_top,
      double to_clockwork, uint64_t mcs_time) override;

 private:
  const int64_t kFixedPointFactor = 1000000000L;
  const int64_t kMinDataCount = 1000;

  int64_t to_right_summ_;
  int64_t to_top_summ_;
  int64_t to_clockwork_summ_;
  int64_t data_counter_;
  std::mutex data_lock_;

  // Путь к файлу с конфигурационными данными
  std::string config_fname_;
};

#endif  // PSVRHELMETCALIBRATION_H
