#include "vr_helmet_calibration.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include <errno.h>

#include "iniparser.h"
#include "home-dir.h"

const char kConfigFileName[] = "/psvrplayer.cfg";

PsvrHelmetCalibration::PsvrHelmetCalibration()
    : to_right_summ_(0),
      to_top_summ_(0),
      to_clockwork_summ_(0),
      data_counter_(0) {
  config_fname_ = HomeDirLibrary::GetDataDir();
  if (config_fname_.empty()) {
    // Путь для конфигурационных данных неопределён
    std::cerr
        << "ERROR: Can't get folder for configuration file. Calibration failed"
        << std::endl;
    throw std::runtime_error("Can't get data folder ");
  }
  config_fname_ += kConfigFileName;
  // Create configuration file if it don't exist
  std::ifstream f1(config_fname_, std::ios_base::in);
  if (!f1) {
    std::ofstream f2(config_fname_, std::ios_base::out | std::ios_base::trunc);
    if (f2.is_open()) {
      f2 << "# ps vr player configuration file" << std::endl;
    } else {
      // Файл не существует и создать его нельзя
      std::cerr << "ERROR: Can't create configuration file '" << config_fname_
                << "'. Calibration failed" << std::endl;
      throw std::runtime_error("Can't create configuration file");
    }
  } else {
    f1.close();
  }
}

bool PsvrHelmetCalibration::DoneCalibration() {
  int64_t tr, tt, tc, dc;
  std::unique_lock<std::mutex> lk(data_lock_);
  tr = to_right_summ_;
  tt = to_top_summ_;
  tc = to_clockwork_summ_;
  dc = data_counter_;
  lk.unlock();

  if (dc < kMinDataCount) {
    std::cerr << "ERROR: Low data from helmet. Calibration failed" << std::endl;
    return false;
  }

  auto dict = iniparser_load(config_fname_.c_str());
  if (!dict) {
    std::cerr << "Can't parse configuration file. Calibration failed"
              << std::endl;
  } else {
    bool res = true;
    auto str = std::to_string(tr / dc);
    auto stt = std::to_string(tt / dc);
    auto stc = std::to_string(tc / dc);
    res = res && (iniparser_set(dict, "Calibration", nullptr) == 0);
    res = res && (iniparser_set(dict, "Calibration:right", str.c_str()) == 0);
    res = res && (iniparser_set(dict, "Calibration:top", stt.c_str()) == 0);
    res = res && (iniparser_set(dict, "Calibration:clock", stc.c_str()) == 0);
    if (!res) {
      std::cerr << "Can't generate configuration file. Calibration failed"
                << std::endl;
    } else {
      auto f = fopen(config_fname_.c_str(), "w+");
      if (!f) {
        std::cerr << "Can't open configuration file '" << config_fname_
                  << "'. Calibration failed" << std::endl;
      } else {
        iniparser_dump_ini(dict, f);
        fclose(f);
        std::cout << "Calibration completed" << std::endl;
        return true;
      }
    }
    iniparser_freedict(dict);
  }
  return false;
}

bool PsvrHelmetCalibration::IsDataAvailable() {
  int64_t dc = 0;
  std::unique_lock<std::mutex> lk(data_lock_);
  dc = data_counter_;
  lk.unlock();
  return dc > kMinDataCount;
}


void PsvrHelmetCalibration::OnSensorsData(
    double to_right, double to_top, double to_clockwork, uint64_t) {
  std::lock_guard<std::mutex> lk(data_lock_);
  to_right_summ_ += to_right * kFixedPointFactor;
  to_top_summ_ += to_top * kFixedPointFactor;
  to_clockwork_summ_ += to_clockwork * kFixedPointFactor;
  ++data_counter_;
}
