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


void SetOptions(std::string* screen, int* eyes_distance) {
  if (!CreateConfigFileIfNotExist(false)) {
    return;
  }
  auto fname = GetConfigFileName();
  auto dict = iniparser_load(fname.c_str());
  if (!dict) {
    std::cerr << "Can't parse configuration file" << std::endl;
  } else {
    if (screen) {
      std::string pe = std::to_string(*eyes_distance);
      iniparser_set(dict, "screen", screen->c_str());
      iniparser_set(dict, "eyes_distance", pe.c_str());
    }
    iniparser_freedict(dict);
  }
}

void GetOptions(std::string* screen, int* eyes_distance) {
  auto fname = GetConfigFileName();
  auto dict = iniparser_load(fname.c_str());
  if (!dict) {
    if (screen) {
      screen->clear();
    }
    if (eyes_distance) {
      *eyes_distance = kDefaultEyesDistance;
    }
  } else {
    // --- Screen
    if (screen) {
      auto ps = iniparser_getstring(dict, "screen", nullptr);
      if (ps) {
        *screen = ps;
      } else {
        screen->clear();
      }
    }

    // --- Eyes Distance
    if (eyes_distance) {
      *eyes_distance =
          iniparser_getint(dict, "eyes_distance", kDefaultEyesDistance);
    }
    iniparser_freedict(dict);
  }
}
