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

#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/perpendicular.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <hidapi/hidapi.h>

#include "vr_helmet_calibration.h"
#include "vr_helmet_view.h"


std::shared_ptr<IHelmet> CreateHelmetView() {
  try {
    return std::shared_ptr<PsvrHelmetView>(new PsvrHelmetView);
  } catch (...) {
  }
  return std::shared_ptr<IHelmet>();
}


std::shared_ptr<IHelmet> CreateHelmetCalibration() {
  try {
    return std::shared_ptr<PsvrHelmetCalibration>(new PsvrHelmetCalibration);
  } catch (...) {
  }
  return std::shared_ptr<IHelmet>();
}
