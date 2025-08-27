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
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <hidapi.h>

#include "vr_helmet_calibration.h"
#include "vr_helmet_view.h"

#include "ctrl-c.h"

const int kCalibrationCheck_1 =
    300;  //!< интервал для проверки данных, в миллисекундах
const int kCalibrationTimeout_1 =
    30000;  //!< Интервал калибровки сенсоров шлема, в миллисекундах
const int kCalibrationTimeTick = 100;  //!< Интервал обновления при калибровке


std::shared_ptr<IHelmet> CreateHelmetView() {
  try {
    return std::shared_ptr<PsvrHelmetView>(new PsvrHelmetView);
  } catch (...) {
  }
  return std::shared_ptr<IHelmet>();
}


int DoHelmetDeviceCalibration() {
  int res = 0;
  unsigned int cr = CtrlCLibrary::kErrorID;
  try {
    bool stop_event = false;
    cr = CtrlCLibrary::SetCtrlCHandler(
        [&stop_event](CtrlCLibrary::CtrlSignal s) -> bool {
          if (s == CtrlCLibrary::kCtrlCSignal) {
            stop_event = true;
          }
          return false;
        });
    if (cr == CtrlCLibrary::kErrorID) {
      std::cerr << "Can't set Ctrl+C handler. Calibration interraption isn't "
                   "available"
                << std::endl;
    }

    auto vr = std::shared_ptr<PsvrHelmetCalibration>(new PsvrHelmetCalibration);

    int summ = 0;
    while (!stop_event && summ < kCalibrationCheck_1) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kCalibrationTimeTick));
      summ += kCalibrationTimeTick;
    }

    if (!vr->IsDataAvailable()) {
      std::cerr << "Helmet data is not available" << std::endl;
      throw 1;
    }

    while (!stop_event && summ < kCalibrationTimeout_1) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(kCalibrationTimeTick));
      summ += kCalibrationTimeTick;
    }
    if (stop_event) {
      std::cerr << "Calibration has broken by user" << std::endl;
      throw 1;
    }

    if (!vr->DoneCalibration()) {
      std::cerr << "Calibration faiiled" << std::endl;
      throw 1;
    }
    vr.reset();
  } catch (int err) {
    res = err;
  } catch (...) {
    res = 1;
  }
  CtrlCLibrary::ResetCtrlCHandler(cr);
  return res;
}
