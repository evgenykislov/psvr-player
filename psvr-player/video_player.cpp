#ifdef __linux__
#define FIX_POSIX_SIGNAL
#endif

#include "video_player.h"

#include <cassert>
#include <iostream>
#include <memory>

#include <vlc/vlc.h>

#ifdef FIX_POSIX_SIGNAL
#include <signal.h>
#endif


/*! Класс для проигрывания видеофайла: открывает файл, выдаёт очередной кадр,
позволяет управлять потоком воспроизведения (пауза/стоп/перемотка) */
class VideoPlayer: public IVideoPlayer {
 public:
  VideoPlayer();
  virtual ~VideoPlayer();

 private:
  VideoPlayer(const VideoPlayer&) = delete;
  VideoPlayer(VideoPlayer&&) = delete;
  VideoPlayer& operator=(const VideoPlayer&) = delete;
  VideoPlayer& operator=(VideoPlayer&&) = delete;

  libvlc_instance_t* lib_vlc_;
};


std::unique_ptr<IVideoPlayer> CreateVideoPlayer() {
  try {
    return std::unique_ptr<IVideoPlayer>(new VideoPlayer());
  } catch (std::bad_alloc&) {
    std::cerr << "ERROR: Lack of memory" << std::endl;
  } catch (std::runtime_error& err) {
    std::cerr << "ERROR: " << err.what() << std::endl;
  }
  return std::unique_ptr<IVideoPlayer>();
}



VideoPlayer::VideoPlayer(): lib_vlc_(nullptr) {
  // OS specific requirements for vlc library
#ifdef FIX_POSIX_SIGNAL
  // Linux code
  sigset_t sg;
  signal(SIGCHLD, SIG_DFL);
  sigemptyset(&sg);
  sigaddset(&sg, SIGPIPE);
  pthread_sigmask(SIG_BLOCK, &sg, NULL);
#endif

  lib_vlc_ = libvlc_new(0, nullptr);
  if (!lib_vlc_) {
    const char* msg = libvlc_errmsg();
    std::cerr << "ERROR: Can't open vlc library." << std::endl;
    if (msg) {
      std::cerr << "  Description: " << msg << std::endl;
    }
    std::cerr << "  Additional information you can see on page: https://apoheliy.com/psvrplayer-vlc-err" << std::endl;
    throw std::runtime_error("vlc library error");
  }
}


VideoPlayer::~VideoPlayer() {
  assert(lib_vlc_);
  libvlc_release(lib_vlc_);
}
