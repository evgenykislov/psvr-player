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


#ifndef VR_HELMET_H
#define VR_HELMET_H

#include <memory>

class IHelmet {
 public:
  virtual ~IHelmet() {}

  enum class VRMode {
    kSingleScreen = 0,  //!< Плоский экран для 2d фильмов
    kSplitScreen = 1  //!< Разделение экрана для 3d фильмов
  };

  virtual void SetVRMode(VRMode mode) = 0;
  virtual void CenterView() = 0;
  // TODO ?? description
  virtual void GetViewPoint(
      double& right_angle, double& top_angle, double& clock_angle) = 0;
};

std::shared_ptr<IHelmet> CreateHelmetView();

int DoHelmetDeviceCalibration();

#endif  // VR_HELMET_H
