
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <thread>  // TODO Remove debug include

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

// TODO Add calibration command

const char kHelpMessage[] =
    "3D Movie player for PS VR. Evgeny Kislov, 2024\n"
    "Usage:\n"
    "  psvrplayer [options] command\n"
    "Commands:\n"
    "  --listscreens - show list of available screens with their position and "
    "exit\n"
    "  --help - show this help and exit\n"
    "  --play=<file-name> - play movie from specified file\n"
    "  --show=squares|colorlines - show test figure, calibration image\n"
    "  --version - show version information and exit\n"
    "Options:\n"
    "  --layer=sbs|ou|mono - specify layer configuration\n"
    "  --screen=<position> - specify screen (by position) to play movie\n"
    "  --swapcolor - correct color\n"
    "  --swaplayer - correct order of layers\n"
    "  --vision=full|semi|flat - specify area of vision\n"
    "More information see on https://apoheliy.com/psvrplayer/\n"
    "";

enum CmdCommand {
  kCommandUnspecified,
  kCommandListScreen,
  kCommandHelp,
  kCommandPlay,
  kCommandVersion,
  kCommandCalibration,
  kCommandShow
} cmd_command = kCommandUnspecified;

std::string cmd_play_fname;
std::string cmd_show_figure;

enum CmdLayer { kLayerSbs, kLayerOu, kLayerMono } cmd_layer = kLayerSbs;

std::string cmd_screen;
bool cmd_swap_color = false;
bool cmd_swap_layer = false;

enum CmdVision {
  kVisionFull,
  kVisionSemi,
  kVisionFlat
} cmd_vision = kVisionSemi;

void PrintHelp() { std::cout << kHelpMessage << std::endl; }

/*! Проверяет аргумент (arg) на соответствие заголовку (header). Если есть
совпадение, то в параметре tail возвращается  остаток строки.
\return признак совпадения аргумента и заголовка */
bool CheckArgument(
    const std::string& arg, const char* header, std::string& tail) {
  std::string h(header);
  if (arg.substr(0, h.size()) != h) {
    return false;
  }
  assert(arg.size() >= h.size());
  tail = arg.substr(h.size());
  return true;
}

/*! Разбор командной строки.
\return признак успешного разбора (без ошибок) */
bool ParseCmd(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    std::string tail;

    if (CheckArgument(arg, "--calibration", tail)) {
      if (!tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      if (cmd_command != kCommandUnspecified) {
        std::cerr << "Extra command '" << arg << "'" << std::endl;
        return false;
      }
      cmd_command = kCommandCalibration;
    } else if (CheckArgument(arg, "--help", tail)) {
      if (!tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      if (cmd_command != kCommandUnspecified) {
        std::cerr << "Extra command '" << arg << "'" << std::endl;
        return false;
      }
      cmd_command = kCommandHelp;
    } else if (CheckArgument(arg, "--layer=", tail)) {
      if (tail == "sbs") {
        cmd_layer = kLayerSbs;
      } else if (tail == "ou") {
        cmd_layer = kLayerOu;
      } else if (tail == "mono") {
        cmd_layer = kLayerMono;
      } else {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
    } else if (CheckArgument(arg, "--listscreens", tail)) {
      if (!tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      if (cmd_command != kCommandUnspecified) {
        std::cerr << "Extra command '" << arg << "'" << std::endl;
        return false;
      }
      cmd_command = kCommandListScreen;
    } else if (CheckArgument(arg, "--play=", tail)) {
      if (tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      if (cmd_command != kCommandUnspecified) {
        std::cerr << "Extra command '" << arg << "'" << std::endl;
        return false;
      }
      cmd_play_fname = tail;
      cmd_command = kCommandPlay;

    } else if (CheckArgument(arg, "--screen=", tail)) {
      if (tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      cmd_screen = tail;
    } else if (CheckArgument(arg, "--show=", tail)) {
      if (tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      if (cmd_command != kCommandUnspecified) {
        std::cerr << "Extra command '" << arg << "'" << std::endl;
        return false;
      }
      cmd_show_figure = tail;
      cmd_command = kCommandShow;
    } else if (CheckArgument(arg, "--swapcolor", tail)) {
      if (!tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      cmd_swap_color = true;
    } else if (CheckArgument(arg, "--swaplayer", tail)) {
      if (!tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      cmd_swap_layer = true;
    } else if (CheckArgument(arg, "--version", tail)) {
      if (!tail.empty()) {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
      if (cmd_command != kCommandUnspecified) {
        std::cerr << "Extra command '" << arg << "'" << std::endl;
        return false;
      }
      cmd_command = kCommandVersion;
    } else if (CheckArgument(arg, "--vision=", tail)) {
      if (tail == "full") {
        cmd_vision = kVisionFull;
      } else if (tail == "semi") {
        cmd_vision = kVisionSemi;
      } else if (tail == "flat") {
        cmd_vision = kVisionFlat;
      } else {
        std::cerr << "Unknown argument '" << arg << "'" << std::endl;
        return false;
      }
    } else {
      std::cerr << "Unknown argument '" << arg << "'" << std::endl;
      return false;
    }
  }
  return true;
}


/*! Выполнить команду play
\return код возврата. 0 - если нет ошибок */
int DoPlayCommand() {
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
  trf->SetEyesDistance(66);

  auto vp = CreateVideoPlayer();
  if (!vp) {
    return 1;
  }

  if (!vp->OpenMovie(cmd_play_fname)) {
    std::cerr << "Can't open movie" << std::endl;
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


/*! Выполнить команду show
\return код возврата. 0 - если нет ошибок */
int DoShowCommand() {
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

  std::atomic_bool stop_show(false);
  std::condition_variable stop_var;
  std::thread show_thread([&stop_show, &stop_var, trf]() {
    int r, g, b;
    r = g = b = 128;
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

        auto f = RequestFrame(1000, 1000);
        f.SetSize(1000, 1000);

        for (int width = 250; width <= 1000; width += 250) {
          f.DrawRectangle(
              500 - width / 2, 500 - width / 2, width, 25, r, g, b, 255);
          f.DrawRectangle(
              500 - width / 2, 475 + width / 2, width, 25, r, g, b, 255);
          f.DrawRectangle(
              500 - width / 2, 525 - width / 2, 25, width - 50, r, g, b, 255);
          f.DrawRectangle(
              475 + width / 2, 525 - width / 2, 25, width - 50, r, g, b, 255);
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
  if (!ParseCmd(argc, argv)) {
    std::cerr << "-----" << std::endl;
    PrintHelp();
    return 1;
  }

  if (cmd_command == kCommandUnspecified) {
    std::cerr << "A command must be specified" << std::endl;
    std::cerr << "-----" << std::endl;
    PrintHelp();
    return 1;
  }

  if (cmd_command == kCommandHelp) {
    PrintHelp();
    return 0;
  }
  if (cmd_command == kCommandVersion) {
    std::cout << kVersion << std::endl;
    return 0;
  }
  if (cmd_command == kCommandListScreen) {
    return PrintMonitors();
  }
  if (cmd_command == kCommandCalibration) {
    auto vr = CreateHelmetCalibration();
    std::this_thread::sleep_for(std::chrono::seconds(kCalibrationTimeout));
    return 0;
  }
  if (cmd_command == kCommandShow) {
    DoShowCommand();
    return 0;
  }
  if (cmd_command == kCommandPlay) {
    return DoPlayCommand();
  }

  return 0;
}
