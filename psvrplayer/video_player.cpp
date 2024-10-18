#ifdef __linux__
#define FIX_POSIX_SIGNAL
#endif

#include "video_player.h"

#include <vlc/vlc.h>

#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>

#ifdef FIX_POSIX_SIGNAL
#include <signal.h>
#endif

#include "framepool.h"


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
  bool Play() override;
  void SetDisplayFn(std::function<void(Frame&&)> fn) override;
  void Pause(bool pause) override;
  void Move(int movement) override;


 private:
  VideoPlayer(const VideoPlayer&) = delete;
  VideoPlayer(VideoPlayer&&) = delete;
  VideoPlayer& operator=(const VideoPlayer&) = delete;
  VideoPlayer& operator=(VideoPlayer&&) = delete;

  const size_t kFramePoolSize =
      4;  //!< Количество предвариательно созданных фреймов
  const size_t kFramePoolSizeAlarm =
      10;  //!< Количество кадров в обработке, когда уже начинается сигнализация
           //!< переполнения

  libvlc_instance_t* lib_vlc_;
  libvlc_media_t* movie_media_;
  libvlc_media_player_t* movie_player_;
  std::mutex lib_lock_;  //!< Блокировка на доступ к объектам vlc библиотеки

  // Переменные из колбэков vlc lib
  unsigned
      video_line_size_;  //!< Размер одной линии в байтах. Должен быть кратна 32
  unsigned
      video_line_width_;  //!< Выровненный размер линии изображения в пикселях
  unsigned video_lines_amount_;  //!< Количество линии. Должно быть кратна 32

  std::atomic<IVideoPlayer::MovieState> movie_state_;

  unsigned video_width_;  //!< Ширина видеопотока в пикселях
  unsigned video_height_;  //!< Высота видеопотока в пикселях
  std::function<void(Frame&&)> on_display_;
  std::mutex on_display_lock_;

  // Функционал фреймов используется для асинхронной выдачи фреймов:
  // готовится фрейм в одном месте, вызов на отображение идёт в другом месте
  // Теоретически возможно, что vlc подготовит сразу несколько фреймов и потом
  // выдаст команды на вывод вразнобой.
  std::vector<Frame> frames_;
  std::mutex frames_lock_;


  /*! Закрыть видеофайл. Внутренняя реализация без привязки в интерфейсу
   * IVideoPlayer */
  void CloseMovieIntr();

  void OnMediaParsed(const struct libvlc_event_t* p_event);

  void* OnVideoBufferLock(void** planes);
  void OnVideoBufferUnlock(void* picture, void* const* planes);
  void OnVideoBufferDisplay(void* picture);
  unsigned OnVideoFormat(char* chroma, unsigned* width, unsigned* height,
      unsigned* pitches, unsigned* lines);
  void OnVideoCleanup();

  // Raw Callbacks
  static void OnMediaParsedRaw(
      const struct libvlc_event_t* p_event, void* p_data) {
    assert(p_data);
    reinterpret_cast<VideoPlayer*>(p_data)->OnMediaParsed(p_event);
  }
  static void* OnVideoBufferLockRaw(void* opaque, void** planes) {
    assert(opaque);
    return reinterpret_cast<VideoPlayer*>(opaque)->OnVideoBufferLock(planes);
  }
  static void OnVideoBufferUnlockRaw(
      void* opaque, void* picture, void* const* planes) {
    assert(opaque);
    reinterpret_cast<VideoPlayer*>(opaque)->OnVideoBufferUnlock(
        picture, planes);
  }
  static void OnVideoBufferDisplayRaw(void* opaque, void* picture) {
    assert(opaque);
    reinterpret_cast<VideoPlayer*>(opaque)->OnVideoBufferDisplay(picture);
  }
  static unsigned OnVideoFormatRaw(void** opaque, char* chroma, unsigned* width,
      unsigned* height, unsigned* pitches, unsigned* lines) {
    assert(opaque);
    assert(*opaque);
    return reinterpret_cast<VideoPlayer*>(*opaque)->OnVideoFormat(
        chroma, width, height, pitches, lines);
  }
  static void OnVideoCleanupRaw(void* opaque) {
    assert(opaque);
    return reinterpret_cast<VideoPlayer*>(opaque)->OnVideoCleanup();
  }
};


IVideoPlayerPtr CreateVideoPlayer() {
  try {
    return std::shared_ptr<IVideoPlayer>(new VideoPlayer());
  } catch (std::bad_alloc&) {
    std::cerr << "ERROR: Lack of memory" << std::endl;
  } catch (std::runtime_error& err) {
    std::cerr << "ERROR: " << err.what() << std::endl;
  }
  return std::unique_ptr<IVideoPlayer>();
}


VideoPlayer::VideoPlayer()
    : lib_vlc_(nullptr),
      movie_media_(nullptr),
      movie_player_(nullptr),
      video_line_size_(0),
      video_line_width_(0),
      video_lines_amount_(0),
      movie_state_(IVideoPlayer::MovieState::kNoMovie),
      video_width_(0),
      video_height_(0) {
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
    std::cerr << "  Additional information you can see on page: "
                 "https://apoheliy.com/psvrplayer-vlc-err"
              << std::endl;
    throw std::runtime_error("vlc library error");
  }

  movie_player_ = libvlc_media_player_new(lib_vlc_);
  if (!movie_player_) {
    std::cerr << "ERROR: Can't create new player" << std::endl;
    const char* msg = libvlc_errmsg();
    if (msg) {
      std::cerr << "  Description: " << msg << std::endl;
    }

    libvlc_release(lib_vlc_);
    throw std::runtime_error("vlc player error");
  }

  libvlc_video_set_callbacks(movie_player_, OnVideoBufferLockRaw,
      OnVideoBufferUnlockRaw, OnVideoBufferDisplayRaw, this);
  libvlc_video_set_format_callbacks(
      movie_player_, OnVideoFormatRaw, OnVideoCleanupRaw);
}


VideoPlayer::~VideoPlayer() {
  assert(lib_vlc_);
  assert(movie_player_);
  CloseMovieIntr();
  libvlc_media_player_release(movie_player_);
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
  res = libvlc_event_attach(
      em, libvlc_MediaParsedChanged, OnMediaParsedRaw, this);
  if (res) {
    std::cerr << "ERROR: Can't process movie file (parsed event error)"
              << std::endl;
    return false;
  }

  res = libvlc_media_parse_with_options(
      movie_media_, libvlc_media_parse_network, 0);
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


void VideoPlayer::CloseMovie() { CloseMovieIntr(); }


IVideoPlayer::MovieState VideoPlayer::GetMovieState() { return movie_state_; }


void VideoPlayer::CloseMovieIntr() {
  assert(movie_player_);
  std::unique_lock<std::mutex> lk(lib_lock_);
  libvlc_media_player_stop(movie_player_);

  if (movie_media_) {
    if (movie_state_ == IVideoPlayer::MovieState::kMovieParsing) {
      libvlc_media_parse_stop(movie_media_);
    }
    libvlc_media_release(movie_media_);
    movie_media_ = nullptr;
  }
  movie_state_ = IVideoPlayer::MovieState::kNoMovie;

  libvlc_media_player_set_media(movie_player_, nullptr);
}


bool VideoPlayer::Play() {
  std::unique_lock<std::mutex> lk(lib_lock_);
  if (!movie_media_) {
    std::cerr << "Movie isn't opened" << std::endl;
    return false;
  }
  if (movie_state_ != IVideoPlayer::MovieState::kMovieReadyToPlay) {
    std::cerr << "Movie hasn't parsed" << std::endl;
    return false;
  }

  libvlc_media_player_set_media(movie_player_, movie_media_);

  unsigned w, h;
  if (libvlc_video_get_size(movie_player_, 0, &w, &h) != 0) {
    std::cerr << "Movie hasn't correct width and height" << std::endl;
    return false;
  }

  std::unique_lock<std::mutex> lk1(on_display_lock_);
  video_width_ = w;
  video_height_ = h;
  lk1.unlock();

  int res = libvlc_media_player_play(movie_player_);
  if (res) {
    std::cerr << "ERROR: Can't play movie" << std::endl;
    const char* msg = libvlc_errmsg();
    if (msg) {
      std::cerr << "  Description: " << msg << std::endl;
    }
    return false;
  }

  return true;
}

void VideoPlayer::SetDisplayFn(std::function<void(Frame&&)> fn) {
  std::lock_guard<std::mutex> lk(on_display_lock_);
  on_display_ = fn;
}

void VideoPlayer::Pause(bool pause) {
  std::unique_lock<std::mutex> lk(lib_lock_);
  libvlc_media_player_set_pause(movie_player_, pause);
}


void VideoPlayer::Move(int movement) {
  std::unique_lock<std::mutex> lk(lib_lock_);
  auto len = libvlc_media_player_get_length(movie_player_);
  if (len == -1) {
    return;
  }
  auto cur = libvlc_media_player_get_time(movie_player_);
  if (cur == -1) {
    return;
  }

  auto newpos = cur + static_cast<libvlc_time_t>(movement) * 1000;
  if (newpos < 0) {
    newpos = 0;
  }
  if (newpos > len) {
    newpos = len;
  }

  if (newpos == cur) {
    // Позиция не меняется. Обычно это происходит по окончании фильма
    return;
  }

  if (!libvlc_media_player_is_playing(movie_player_)) {
    libvlc_media_player_set_media(movie_player_, nullptr);
    libvlc_media_player_set_media(movie_player_, movie_media_);
    int res = libvlc_media_player_play(movie_player_);
    if (res) {
      std::cerr << "ERROR: Can't restart movie playing" << std::endl;
      const char* msg = libvlc_errmsg();
      if (msg) {
        std::cerr << "  Description: " << msg << std::endl;
      }
      return;
    }
  }

  libvlc_media_player_set_time(movie_player_, newpos);
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


void* VideoPlayer::OnVideoBufferLock(void** planes) {
  Frame fr = RequestFrame(video_line_width_, video_lines_amount_);
  fr.SetSize(video_width_, video_height_);
  size_t sz;
  *planes = fr.GetData(sz);
  std::lock_guard<std::mutex> lk(frames_lock_);
  frames_.push_back(std::move(fr));
  if (frames_.size() > kFramePoolSizeAlarm) {
    std::cerr << "Frame processing OVERFLOW" << std::endl;
  }
  return *planes;
}


// TODO2 Remove?
void VideoPlayer::OnVideoBufferUnlock(void*, void* const*) {}


void VideoPlayer::OnVideoBufferDisplay(void* picture) {
  // Найдём фрейм по адресу блока данных
  // Все игрища с поиском, указателями и т.д.
  // нужны для корректной обработки схемы "у фрейма один владелец"
  std::unique_lock<std::mutex> lk(frames_lock_);
  int index = -1;
  for (int i = 0; i < (int)frames_.size(); ++i) {
    Frame& fr = frames_[i];
    size_t sz;
    if (fr.GetData(sz) == picture) {
      index = i;
      break;
    }
  }
  assert(index != -1);
  if (index == -1) {
    std::cerr << "LOGIC_ERROR: frame has'n found" << std::endl;
    return;
  }

  Frame fr = std::move(frames_[index]);
  frames_.erase(frames_.begin() + index);
  lk.unlock();

  std::lock_guard<std::mutex> dl(on_display_lock_);
  if (on_display_) {
    on_display_(std::move(fr));
  }
}


unsigned VideoPlayer::OnVideoFormat(char* chroma, unsigned* width,
    unsigned* height, unsigned* pitches, unsigned* lines) {
  // Получаем формат видеофайла и можем выдать подходящий нам новый формат
  try {
    *pitches = video_line_size_ = ((*width * 4 + 31) / 32) * 32;
    *lines = video_lines_amount_ = ((*height + 31) / 32) * 32;
    video_line_width_ = video_line_size_ / 4;
    std::memcpy(chroma, "RV32", 4);  // Копируем только 4 байта, без нулевого

    // Заполним пул фреймов: создадим kFramePoolSize фреймов и потом освободим
    std::vector<Frame> cache;
    cache.reserve(kFramePoolSize);
    for (size_t i = 0; i < kFramePoolSize; ++i) {
      cache.push_back(RequestFrame(video_line_width_, video_lines_amount_));
    }
    while (!cache.empty()) {
      ReleaseFrame(std::move(cache.back()));
      cache.pop_back();
    }

    return kFramePoolSize;
  } catch (std::bad_alloc&) {
  }

  return 0;
}

void VideoPlayer::OnVideoCleanup() {
  std::unique_lock<std::mutex> lk(frames_lock_);
  while (!frames_.empty()) {
    ReleaseFrame(std::move(frames_.back()));
    frames_.pop_back();
  }
  lk.unlock();
}
