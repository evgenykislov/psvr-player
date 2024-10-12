#ifndef PSVRHELMETCALIBRATION_H
#define PSVRHELMETCALIBRATION_H

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
};

#endif  // PSVRHELMETCALIBRATION_H
