#include "playing.h"

#include <chrono>
#include <iostream>

// clang-format off
// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// clang-format on

#include "transformer.h"
#include "video_player.h"


int scancode_space_ = 65;
int scancode_left_arrow_ = 113;
int scancode_right_arrow_ = 114;

bool pause_state_ = false;

struct KeySeq {
  std::chrono::steady_clock::time_point
      pre_press;  //!< Время предыдущего нажатия
  std::chrono::steady_clock::time_point
      pre_pre_press;  //!< Время пред-предыдущего нажатия
};

const auto kMultiKeyInterval = std::chrono::milliseconds(1000);

enum MultiKey { kSingleKey, kDoubleKey, kTripleKey };


KeySeq left_arrow_seq_;
KeySeq right_arrow_seq_;


bool first_value = true;
double central_x;
double central_y;
double last_x;
double last_y;


void KeyAction(KeySeq& seq, MultiKey& multi) {
  auto cur = std::chrono::steady_clock::now();
  auto cur_mi = cur - kMultiKeyInterval;
  if (seq.pre_pre_press >= cur_mi && seq.pre_pre_press < cur) {
    multi = kTripleKey;
  } else if (seq.pre_press >= cur_mi && seq.pre_press < cur) {
    multi = kDoubleKey;
  } else {
    multi = kSingleKey;
  }

  seq.pre_pre_press = seq.pre_press;
  seq.pre_press = cur;
}

int GetMovement(MultiKey m) {
  if (m == kTripleKey) {
    return 300;
  }
  if (m == kDoubleKey) {
    return 30;
  }
  return 5;
}

void KeyProcessor(int key, int scancode, int action, int mods,
    std::shared_ptr<IVideoPlayer> player) {
  std::cout << "Key: " << key << ", scan " << scancode << ", action " << action
            << ", mods " << mods << std::endl;
  if (scancode == scancode_space_ && action == GLFW_PRESS && mods == 0) {
    pause_state_ = !pause_state_;
    player->Pause(pause_state_);

    // Additional alignment action
    central_x = last_x;
    central_y = last_y;

    return;
  }

  if (scancode == scancode_left_arrow_ && action == GLFW_PRESS && mods == 0) {
    MultiKey m;
    KeyAction(left_arrow_seq_, m);
    player->Move(-GetMovement(m));
    return;
  }

  if (scancode == scancode_right_arrow_ && action == GLFW_PRESS && mods == 0) {
    MultiKey m;
    KeyAction(right_arrow_seq_, m);
    player->Move(GetMovement(m));
  }
}


void MouseProcessor(double x_pos, double y_pos, Transformer* transformer) {
  if (first_value) {
    central_x = x_pos;
    central_y = y_pos;
    first_value = false;
  }

  last_x = x_pos;
  last_y = y_pos;

  const float kMouseScale = 1.0f / 600.0f;
  transformer->SetViewPoint(float(x_pos - central_x) * kMouseScale,
      float(y_pos - central_y) * kMouseScale);
}
