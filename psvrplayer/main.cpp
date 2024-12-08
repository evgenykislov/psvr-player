
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <map>
#include <thread>  // TODO Remove debug include

#include "config_file.h"
#include "framepool.h"
#include "monitors.h"
#include "play_screen.h"
#include "playing.h"
#include "transformer.h"
#include "version.h"
#include "video_player.h"
#include "vr_helmet.h"

const int kCalibrationTimeout =
    20;  //!< Интервал калибровки сенсоров шлема, в секундах


// clang-format off
const char kHelpMessage[] =
    "3D Movie player for PS VR. Evgeny Kislov, 2024\n"
    "Usage:\n"
    "  psvrplayer command [options] [moviefile ..]\n"
    "Commands:\n"
    "  --calibration - calibrate vr helmet device\n"
    "  --listscreens - show list of available screens with their position\n"
    "  --help - show this help\n"
    "  --play=<file-name> - play movie from specified file\n"
    "  --save - save current options as default\n"
    "  --show=squares|colorlines - show test calibration image\n"
    "  --version - show version information\n"
    "Options:\n"
    "  --eyes=<distance> - specify eyes distance\n"
    /*    "  --layer=sbs|ou|mono - specify layer configuration\n" */
    "  --screen=<position> - specify screen (by position) to play movie\n"
    "  --swapcolor - correct color\n"
    "  --swaplayer - correct order of layers\n"
    /*    "  --vision=full|semi|flat - specify area of vision\n" */
    "More information see on https://apoheliy.com/psvrplayer/\n"
    "";
// clang-format on

enum ParamType { kEmptyValue, kStringValue, kNumberValue };

enum ParamCmd {
  kCmdCalibration,
  kCmdEyes,
  kCmdHelp,
  kCmdLayer,
  kCmdListScreens,
  kCmdPlay,
  kCmdSave,
  kCmdScreen,
  kCmdShow,
  kCmdSwapColor,
  kCmdSwapLayer,
  kCmdVersion,
  kCmdVision
};

const std::string kCmdPrefix = "--";
const ParamCmd kEmptyPrefixCmd = kCmdPlay;

struct CommandLineParam {
  ParamCmd cmd;  // Выполняемая команда или изменяемый параметр
  bool is_command;  // Признак, что параметр является командой (один тип на
                    // командную строку)
  bool is_multiset;  // Признак, что параметр можно указывать несколько раз
  ParamType value_type;     // Тип параметра
  std::string prefix;       // Заголовок параметра
  std::string description;  // Описание параметра
};

struct CommandLineValue {
  std::string strvalue;  // Строковое значение параметра
  int numvalue;  // Числовое значение параметра (если выставлен numtype)
};

// clang-format off
std::array<CommandLineParam, 13> CmdParameters = {{
  {kCmdCalibration, true, false, kEmptyValue, "--calibration", "calibration command"},
  {kCmdEyes, false, false, kNumberValue, "--eyes=", "interpupillary distance"},
  {kCmdHelp, true, false, kEmptyValue, "--help", "help command"},
  {kCmdLayer, false, false, kStringValue, "--layer=", "layer switcher"},
  {kCmdListScreens, true, false, kEmptyValue, "--listscreens", "list screens command"},
  {kCmdPlay, true, true, kStringValue, "--play=", "play movie file"},
  {kCmdSave, true, false, kEmptyValue, "--save", "save current option"},
  {kCmdScreen, false, false, kStringValue, "--screen=", "select screen"},
  {kCmdShow, true, false, kStringValue, "--show=", "show test images"},
  {kCmdSwapColor, false, false, kEmptyValue, "--swapcolor", "change color palette"},
  {kCmdSwapLayer, false, false, kEmptyValue, "--swaplayer", "swap left/right view"},
  {kCmdVersion, false, false, kEmptyValue, "--version", "show version information"},
  {kCmdVision, false, false, kStringValue, "--vision=", "selects format of 3D movie"},
}};
// clang-format on

std::map<ParamCmd, std::vector<CommandLineValue>> CmdValues;

enum CmdLayer { kLayerSbs, kLayerOu, kLayerMono } cmd_layer = kLayerSbs;
std::string cmd_screen;
bool cmd_swap_color = false;
bool cmd_swap_layer = false;
int cmd_eyes_distance = 0;

enum CmdVision {
  kVisionFull,
  kVisionSemi,
  kVisionFlat
} cmd_vision = kVisionSemi;


/*! Найти первую непустую команду в CmdValues
\return количество непустых команд в CmdValues
*/
int FindFirstCmd(ParamCmd& cmd) {
  int amount = 0;
  for (auto it = CmdValues.begin(); it != CmdValues.end(); ++it) {
    if (it->second.empty()) {
      continue;
    }
    // Немного непроизводительно, но параметров мало
    bool iscmd = false;
    for (auto cit = CmdParameters.begin(); cit != CmdParameters.end(); ++cit) {
      if (cit->cmd == it->first && cit->is_command) {
        iscmd = true;
        break;
      }
    }
    if (!iscmd) {
      continue;
    }

    // Это команда и она не пустая
    if (amount == 0) {
      cmd = it->first;
    }
    ++amount;
  }
  return amount;
}

/*! Разбор командной строки.
\return признак успешного разбора (без ошибок) */
bool ParseCommandLine(int argc, char** argv) {
  CmdValues.clear();
  for (int i = 1; i < argc; ++i) {
    auto param_it = CmdParameters.end();
    std::string param_tail = argv[i];
    if (param_tail.substr(0, kCmdPrefix.size()) != kCmdPrefix) {
      for (auto cit = CmdParameters.begin(); cit != CmdParameters.end();
           ++cit) {
        if (cit->cmd == kEmptyPrefixCmd) {
          param_it = cit;
        }
      }
    } else {
      for (auto cit = CmdParameters.begin(); cit != CmdParameters.end();
           ++cit) {
        if (param_tail.substr(0, cit->prefix.size()) == cit->prefix) {
          param_it = cit;
          param_tail = param_tail.substr(cit->prefix.size());
        }
      }
    }

    if (param_it == CmdParameters.end()) {
      std::cerr << "Unknown parameter '" << param_tail << "'" << std::endl;
      return false;
    }

    CommandLineValue val;
    val.strvalue = param_tail;
    val.numvalue = 0;
    switch (param_it->value_type) {
      case kEmptyValue:
        if (!param_tail.empty()) {
          std::cerr << "Wrong format of parameter '" << param_it->prefix << "'"
                    << std::endl;
          return false;
        }
        break;
      case kStringValue:
        if (param_tail.empty()) {
          std::cerr << "Empty value for parameter '" << param_it->prefix << "'"
                    << std::endl;
          return false;
        }
        break;
      case kNumberValue:
        try {
          val.numvalue = std::stoi(param_tail);
        } catch (...) {
          std::cerr << "Wrong numerical value for parameter '"
                    << param_it->prefix << "'" << std::endl;
          return false;
        }
        break;
      default:
        assert(false);
    }

    CmdValues[param_it->cmd].push_back(val);
    if (!param_it->is_multiset && CmdValues[param_it->cmd].size() > 1) {
      std::cerr << "Many values for parameter '" << param_it->prefix << "'"
                << std::endl;
      return false;
    }
    ParamCmd cmd;
    if (FindFirstCmd(cmd) > 1) {
      std::cerr << "Parameter '" << param_it->prefix << "' adds extra command"
                << std::endl;
      return false;
    }
  }

  return true;
}


/*! Проверить параметры командной строки на корректность. Заполнить
дополнительные переменные.
\return Признак сформированных корректных параметров */
bool CheckParameters() {
  decltype(CmdValues.end()) l;
  l = CmdValues.find(kCmdLayer);
  if (l != CmdValues.end() && !l->second.empty()) {
    auto v = l->second[0].strvalue;
    if (v == "sbs") {
      cmd_layer = kLayerSbs;
    } else if (v == "ou") {
      cmd_layer = kLayerOu;
    } else if (v == "mono") {
      cmd_layer = kLayerMono;
    } else {
      std::cerr << "Unknown layer type '" << v << "'" << std::endl;
      return false;
    }
  }

  l = CmdValues.find(kCmdScreen);
  if (l != CmdValues.end() && !l->second.empty()) {
    cmd_screen = l->second[0].strvalue;
  }

  if (CmdValues.find(kCmdSwapColor) != CmdValues.end()) {
    cmd_swap_color = true;
  }

  if (CmdValues.find(kCmdSwapLayer) != CmdValues.end()) {
    cmd_swap_layer = true;
  }

  l = CmdValues.find(kCmdEyes);
  if (l != CmdValues.end() && !l->second.empty()) {
    cmd_eyes_distance = l->second[0].numvalue;
  }

  l = CmdValues.find(kCmdVision);
  if (l != CmdValues.end() && !l->second.empty()) {
    auto v = l->second[0].strvalue;
    if (v == "full") {
      cmd_vision = kVisionFull;
    } else if (v == "semi") {
      cmd_vision = kVisionSemi;
    } else if (v == "flat") {
      cmd_vision = kVisionFlat;
    } else {
      std::cerr << "Unknown vision type '" << v << "'" << std::endl;
      return false;
    }
  }

  return true;
}


void PrintHelp() { std::cout << kHelpMessage << std::endl; }


/*! Выполнить команду play
\return код возврата. 0 - если нет ошибок */
int DoPlayCommand(std::string fname) {
  auto vr = CreateHelmetView();
  if (!vr) {
    std::cerr << "PS VR Helmet not found" << std::endl;
  }

  if (vr) {
    bool vr_mode = (cmd_layer == kLayerSbs) || (cmd_layer == kLayerOu);
    vr->SetVRMode(vr_mode ? IHelmet::VRMode::kSplitScreen
                          : IHelmet::VRMode::kSingleScreen);
  }

  auto ps = CreatePlayScreen(cmd_screen);
  if (!ps) {
    return 1;
  }

  auto trf = CreateTransformer(kLeftRight180, ps, vr);
  if (!trf) {
    return 1;
  }

  trf->SetEyeSwap(cmd_swap_layer);
  trf->SetEyesDistance(cmd_eyes_distance);

  auto vp = CreateVideoPlayer();
  if (!vp) {
    return 1;
  }

  if (!vp->OpenMovie(fname)) {
    std::cerr << "Can't open movie '" << fname << "'" << std::endl;
  }

  while (vp->GetMovieState() == IVideoPlayer::MovieState::kMovieParsing) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // TODO Remove debug
  vp->SetDisplayFn([&trf](Frame&& frame) { trf->SetImage(std::move(frame)); });

  vp->Play();

  if (ps) {
    ps->SetKeyboardFilter(
        [vp, vr](int key, int scancode, int action, int mods) {
          KeyProcessor(key, scancode, action, mods, vp, vr);
        });
    ps->SetMouseEvent([trf](double x_pos, double y_pos) {
      MouseProcessor(x_pos, y_pos, trf);
    });
    ps->Run();
  }

  // TODO Сделать корректное освобождение ресурсов
  // delete trf;
  return 0;
}


Frame GenerateSquares() {
  int r, g, b;
  r = g = b = 128;
  auto f = RequestFrame(1000, 1000);
  f.SetSize(1000, 1000);

  for (int width = 250; width <= 1000; width += 250) {
    f.DrawRectangle(500 - width / 2, 500 - width / 2, width, 25, r, g, b, 255);
    f.DrawRectangle(500 - width / 2, 475 + width / 2, width, 25, r, g, b, 255);
    f.DrawRectangle(
        500 - width / 2, 525 - width / 2, 25, width - 50, r, g, b, 255);
    f.DrawRectangle(
        475 + width / 2, 525 - width / 2, 25, width - 50, r, g, b, 255);
  }

  return f;
}


Frame GenerateColors() {
  auto f = RequestFrame(1000, 1000);
  f.SetSize(1000, 1000);

  for (int i = 0; i < 1000; i += 110) {
    for (int j = 0; j < 1000; j += 110) {
      f.DrawRectangle(i, j, 10, 10, 128, 0, 0, 255);
      f.DrawRectangle(i + 10, j, 50, 10, 0, 128, 0, 255);
      f.DrawRectangle(i + 60, j, 50, 10, 0, 0, 128, 255);
      f.DrawRectangle(i, j + 10, 10, 50, 0, 128, 0, 255);
      f.DrawRectangle(i, j + 60, 10, 50, 0, 0, 128, 255);
    }
  }

  return f;
}


/*! Выполнить команду show
\return код возврата. 0 - если нет ошибок */
int DoShowCommand(std::string figure) {
  auto vr = CreateHelmetView();
  if (!vr) {
    std::cerr << "PS VR Helmet not found" << std::endl;
  } else {
    vr->SetVRMode(IHelmet::VRMode::kSplitScreen);
  }

  auto ps = CreatePlayScreen(cmd_screen);
  if (!ps) {
    return 1;
  }

  auto trf = CreateTransformer(kSingleImage, ps, vr);
  if (!trf) {
    return 1;
  }

  trf->SetEyesDistance(cmd_eyes_distance);


  std::atomic_bool stop_show(false);
  std::condition_variable stop_var;
  std::thread show_thread([&stop_show, &stop_var, trf, figure]() {
    int fast_cycle_counter = 20;  //!< Счётчик частых показов (на старте)
    try {
      while (true) {
        std::mutex m;
        std::unique_lock<std::mutex> lk(m);
        stop_var.wait_for(
            lk, std::chrono::milliseconds(fast_cycle_counter > 0 ? 100 : 3000));
        if (fast_cycle_counter > 0) {
          --fast_cycle_counter;
        }

        if (stop_show) {
          break;
        }

        Frame f(1000, 1000);
        if (figure == "squares") {
          f = GenerateSquares();
        } else if (figure == "colors") {
          f = GenerateColors();
        } else {
          std::cerr << "Unknown show figure '" << figure << "'" << std::endl;
          stop_show = true;
        }
        trf->SetImage(std::move(f));
      }
    } catch (std::exception) {
      std::cerr << "Can't create show frame" << std::endl;
    }
  });

  assert(ps);
  ps->Run();

  stop_show = true;
  stop_var.notify_all();
  show_thread.join();

  // TODO Сделать корректное освобождение ресурсов
  // delete trf;
  return 0;
}


int main(int argc, char** argv) {
  GetOptions(&cmd_screen, &cmd_eyes_distance);

  if (!ParseCommandLine(argc, argv)) {
    std::cerr << "--------------------------------------" << std::endl;
    PrintHelp();
    return 1;
  }

  // Дополнительные проверки
  ParamCmd cmd;
  if (FindFirstCmd(cmd) != 1) {
    std::cerr << "Command not specified" << std::endl;
    std::cerr << "--------------------------------------" << std::endl;
    PrintHelp();
    return 1;
  }

  if (!CheckParameters()) {
    std::cerr << "--------------------------------------" << std::endl;
    PrintHelp();
    return 1;
  }

  int res = 0;
  switch (cmd) {
    case kCmdCalibration: {
      auto vr = CreateHelmetCalibration();
      std::this_thread::sleep_for(std::chrono::seconds(kCalibrationTimeout));
    } break;
    case kCmdHelp:
      PrintHelp();
      break;
    case kCmdListScreens:
      res = PrintMonitors();
      break;
    case kCmdSave:
      SetOptions(&cmd_screen, &cmd_eyes_distance);
      break;
    case kCmdPlay: {
      auto l = CmdValues.find(kCmdPlay);
      if (l != CmdValues.end()) {
        for (auto it = l->second.begin(); it != l->second.end(); ++it) {
          auto r = DoPlayCommand(it->strvalue);
          if (res == 0) {
            res = r;
          }
        }
      }
    } break;
    case kCmdShow: {
      auto l = CmdValues.find(kCmdShow);
      if (l != CmdValues.end() && !l->second.empty()) {
        DoShowCommand(l->second[0].strvalue);
      }
    } break;
    case kCmdVersion:
      std::cout << kVersion << std::endl;
      break;
    default:
      assert(false);
  }

  return 0;
}
