#ifdef __linux__
#define FIX_POSIX_SIGNAL
#endif

#include "video_player.h"

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>

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

  // Public API
  bool OpenMovie(const std::string& filename) override;
  void CloseMovie() override;
  IVideoPlayer::MovieState GetMovieState() override;


 private:
  VideoPlayer(const VideoPlayer&) = delete;
  VideoPlayer(VideoPlayer&&) = delete;
  VideoPlayer& operator=(const VideoPlayer&) = delete;
  VideoPlayer& operator=(VideoPlayer&&) = delete;

  libvlc_instance_t* lib_vlc_;
  libvlc_media_t* movie_media_;
  std::mutex lib_lock_; //!< Блокировка на доступ к объектам vlc библиотеки

  std::atomic<IVideoPlayer::MovieState> movie_state_;

  /*! Закрыть видеофайл. Внутренняя реализация без привязки в интерфейсу IVideoPlayer */
  void CloseMovieIntr();

  void OnMediaParsed(const struct libvlc_event_t *p_event);

  // Raw Callbacks
  static void OnMediaParsedRaw(const struct libvlc_event_t *p_event, void *p_data) {
    assert(p_data);
    reinterpret_cast<VideoPlayer*>(p_data)->OnMediaParsed(p_event);
  }

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


VideoPlayer::VideoPlayer(): lib_vlc_(nullptr), movie_media_(nullptr),
    movie_state_(IVideoPlayer::MovieState::kNoMovie){
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
  CloseMovieIntr();
  libvlc_release(lib_vlc_);
}


bool VideoPlayer::OpenMovie(const std::string& filename) {
  int res;
  std::unique_lock<std::mutex> lk(lib_lock_);

  if (movie_media_) {
    std::cerr << "ERROR: Other movie file is opened yet" << std::endl;
    return false;
  }

  movie_media_ = libvlc_media_new_path(lib_vlc_, filename.c_str());
  if (!movie_media_) {
    std::cerr << "Can't open movie file '" << filename << "'" << std::endl;
    return false;
  }

  auto em = libvlc_media_event_manager(movie_media_);
  assert(em);
  res = libvlc_event_attach(em, libvlc_MediaParsedChanged, OnMediaParsedRaw, this);
  if (res) {
    std::cerr << "ERROR: Can't process movie file (parsed event error)" << std::endl;
    return false;
  }

  res = libvlc_media_parse_with_options(movie_media_, libvlc_media_parse_network, 0);
  if (res) {
    const char* msg = libvlc_errmsg();
    std::cerr << "ERROR: Can't parse movie" << std::endl;
    if (msg) {
      std::cerr << "  Description: " << msg << std::endl;
    }
  }

  movie_state_ = IVideoPlayer::MovieState::kMovieParsing;
  return true;
}


void VideoPlayer::CloseMovie() {
  CloseMovieIntr();
}


IVideoPlayer::MovieState VideoPlayer::GetMovieState() {
  return movie_state_;
}

void VideoPlayer::CloseMovieIntr() {
  std::unique_lock<std::mutex> lk(lib_lock_);

  if (movie_media_) {
    if (movie_state_ == IVideoPlayer::MovieState::kMovieParsing) {
      libvlc_media_parse_stop(movie_media_);
    }
    libvlc_media_release(movie_media_);
    movie_media_ = nullptr;
  }
  movie_state_ = IVideoPlayer::MovieState::kNoMovie;
}


void VideoPlayer::OnMediaParsed(const libvlc_event_t* p_event) {
  switch (p_event->u.media_parsed_changed.new_status) {
    case libvlc_media_parsed_status_skipped:
    case libvlc_media_parsed_status_failed:
    case libvlc_media_parsed_status_timeout:
      std::cerr << "Movie parsed with errors" << std::endl;
      movie_state_ = IVideoPlayer::MovieState::kMovieFailed;
      break;
    case libvlc_media_parsed_status_done:
      movie_state_ = IVideoPlayer::MovieState::kMovieReadyToPlay;
      break;
  }
}
