#include "config_file.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <mutex>

#include "home-dir.h"
#include "iniparser.h"


const int kDefaultEyesDistance = 66;  //!< Расстояние между окулярами в шлеме


std::string GetConfigFileName() {
  static std::mutex mt;
  static std::string fname;

  std::lock_guard<std::mutex> lk(mt);
  if (fname.empty()) {
    fname = HomeDirLibrary::GetDataDir() + "/psvrplayer.cfg";
  }
  return fname;
}


bool CreateConfigFileIfNotExist(bool force_clear) {
  auto fname = GetConfigFileName();
  if (fname.empty()) {
    std::cerr << "ERROR: Unknown configuration file" << std::endl;
    return false;
  }

  auto f = std::fopen(fname.c_str(), force_clear ? "w+" : "a+");
  if (!f) {
    std::cerr << "ERROR: Can't create/write configuration file '" << fname
              << "'" << std::endl;
    return false;
  }
  std::fclose(f);
  return true;
}


void SetOptions(std::string* screen, int* eyes_distance, bool* swap_color,
    bool* swap_layer, double* rotation) {
  if (!CreateConfigFileIfNotExist(false)) {
    return;
  }
  auto fname = GetConfigFileName();
  auto dict = iniparser_load(fname.c_str());
  if (!dict) {
    std::cerr << "Can't parse configuration file" << std::endl;
  } else {
    iniparser_set(dict, "Options", nullptr);

    if (screen) {
      iniparser_set(dict, "Options:screen", screen->c_str());
    }

    if (eyes_distance) {
      std::string pe = std::to_string(*eyes_distance);
      iniparser_set(dict, "Options:eyes_distance", pe.c_str());
    }

    if (swap_color) {
      iniparser_set(dict, "Options:swap_color", *swap_color ? "1" : "0");
    }

    if (swap_layer) {
      iniparser_set(dict, "Options:swap_layer", *swap_layer ? "1" : "0");
    }

    if (rotation) {
      std::string pe = std::to_string(*rotation);
      iniparser_set(dict, "Options:rotation_speedup", pe.c_str());
    }

    auto f = fopen(fname.c_str(), "w+");
    if (!f) {
      std::cerr << "Can't open configuration file '" << fname << "'"
                << std::endl;
    } else {
      iniparser_dump_ini(dict, f);
      fclose(f);
      std::cout << "Options are saved" << std::endl;
    }

    iniparser_freedict(dict);
  }
}

void GetOptions(std::string* screen, int* eyes_distance, bool* swap_color,
    bool* swap_layer, double* rotation) {
  auto fname = GetConfigFileName();
  auto dict = iniparser_load(fname.c_str());
  if (!dict) {
    if (screen) {
      screen->clear();
    }
    if (eyes_distance) {
      *eyes_distance = kDefaultEyesDistance;
    }
    if (swap_color) {
      *swap_color = false;
    }
    if (swap_layer) {
      *swap_layer = false;
    }
  } else {
    // --- Screen
    if (screen) {
      *screen = iniparser_getstring(dict, "Options:screen", "");
    }

    // --- Eyes Distance
    if (eyes_distance) {
      *eyes_distance =
          iniparser_getint(dict, "Options:eyes_distance", kDefaultEyesDistance);
    }

    // --- Swap Color
    if (swap_color) {
      *swap_color = iniparser_getint(dict, "Options:swap_color", 0) != 0;
    }

    // --- Swap Layer
    if (swap_layer) {
      *swap_layer = iniparser_getint(dict, "Options:swap_layer", 0) != 0;
    }

    if (rotation) {
      *rotation = iniparser_getdouble(dict, "Options:rotation_speedup", 1.0);
    }

    iniparser_freedict(dict);
  }
}

void ClearOptions() { CreateConfigFileIfNotExist(true); }
