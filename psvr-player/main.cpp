#include <cassert>
#include <iostream>

const char kHelpMessage[] = \
  "PS VR Player. Evgeny Kislov, 2024\n"
  "Usage:\n"
  "  psvrplayer [options] command\n"
  "Commands:\n"
  "  --listscreens\n"
  "  --help\n"
  "  --play=<file-name>\n"
  "  --version\n"
  "Options:\n"
  "  --layer=sbs|ou|mono\n"
  "  --screen=<screen>\n"
  "  --swapcolor\n"
  "  --swaplayer\n"
  "  --vision=full|semi|flat\n";

enum CmdCommand {
  kCommandUnspecified,
  kCommandListScreen,
  kCommandHelp,
  kCommandPlay,
  kCommandVersion
} cmd_command = kCommandUnspecified;

std::string cmd_play_fname;

enum CmdLayer {
  kLayerSbs,
  kLayerOu,
  kLayerMono
} cmd_layer = kLayerSbs;

int cmd_screen = 0;
bool cmd_swap_color = false;
bool cmd_swap_layer = false;

enum CmdVision {
  kVisionFull,
  kVisionSemi,
  kVisionFlat
} cmd_vision = kVisionSemi;


void PrintHelp() {
  std::cout << kHelpMessage << std::endl;
}


/*! Проверяет аргумент (arg) на соответствие заголовку (header). Если есть
совпадение, то в параметре tail возвращается  остаток строки.
\return признак совпадения аргумента и заголовка */
bool CheckArgument(const std::string& arg, const char* header, std::string& tail) {
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
    if (CheckArgument(arg, "--help", tail)) {
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
      if (tail == "sbs") { cmd_layer = kLayerSbs; }
      else if (tail == "ou") { cmd_layer = kLayerOu; }
      else if (tail == "mono") { cmd_layer = kLayerMono; }
      else {
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
      cmd_screen = std::stoi(tail);
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
      if (tail == "full") { cmd_vision = kVisionFull; }
      else if (tail == "semi") { cmd_vision = kVisionSemi; }
      else if (tail == "flat") { cmd_vision = kVisionFlat; }
      else {
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

  std::cout << "Parsing success" << std::endl;
  return 0;
}