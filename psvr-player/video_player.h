#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <functional>
#include <memory>

#include "framepool.h"


/*! Интерфейс для работы с проигрывателем видеофайлов */
class IVideoPlayer {
 public:
  enum class MovieState {
    kNoMovie,
    kMovieParsing,
    kMovieReadyToPlay,
    kMovieFailed
  };

  /*! Открыть видеофайл. Функция только начинает процесс открытия и разбора
  файла, который может занять существенное время.
  Функция потокобезопасная, не выбрасывает исключений, в случае ошибок описание
  пишет в cerr.
  \param filename имя файла для открытия. Может быть как с полным путём, так
  только имя. \return признак успешного открытия файла и начала процедуры
  разбора. Возврат false указывает на ошибку. Возможные ошибки: нет или
  недоступен файл, нераспознанный формат, уже есть открытый файл. */
  virtual bool OpenMovie(const std::string& filename) = 0;

  /*! Закрывает открытый ранее файл. Если файл не был открыт, то функция ничего
  не делает. Если разбор файла ещё не закончился, то процедура разбора
  прерывается и файл закрывается.
  Функция потокобезопасная и не выбрасывает исключений. */
  virtual void CloseMovie() = 0;

  /*! Возвращает текущее состояние файла для проигрывания. Это не состояние
  воспроизведения
  \return текущее состояние файла */
  virtual MovieState GetMovieState() = 0;

  /*! Начинает воспроизведение файла. Файл должен быть открыт и успешно разобран
  (состояние kMovieReadyToPlay). Функция потокобезопасная и не выбрасывает
  исключений.
  \return признак начавшегося воспроизведения. Если файл не готов к
  воспроизведению, то возвращается false */
  virtual bool Play() = 0;

  /*! Приостанавливает или продолжает воспроизведение файла. Функция
  потокобезопасная и не выбрасывает исключений.
  \param pause флаг команды: true - включить паузу, false - продолжить
  воспроизведение */
  virtual void Pause(bool pause) = 0;


  /*! Смещает позицию воспроизведения на заданный интервал времени. Смещение
  допустимо как вперёд (положительное перемещение), так и назад (отрицательное
  перемещение). Функция потокобезопасная и не выбрасывает исключений.
  \param movement интервал смещения воспроизведения в секундах. Для смещения
  назад допустимо задать отрицательное значение. */
  virtual void Move(int movement) = 0;


  /*! Выставляет колбэк на вывод данных покадрово. Если колбэк уже был
  установлен, то при вызове функции предыдущая настройка удаляется.
  \param fn функция обратного вызова для получения данных о кадре. Аргументы
  функции: width (ширина картинки для отображения - реальная ширина из файла в
  пикселях), height (высота картинки в пикселях), align_width (выровненная
  ширина картинки - не меньше реальной ширины), data (данные о пикселях по 4
  байта на пиксель - для выравнивания). Выдаваемое изображение прижато к левому
  краю, в конце каждой линии могут быть мусорные пиксели для выравнивания */
  virtual void SetDisplayFn(std::function<void(Frame&&)> fn) = 0;

  virtual ~IVideoPlayer() {}
};

using IVideoPlayerPtr = std::shared_ptr<IVideoPlayer>;


/*! Функция создания экземпляра видеопроигрывателя
\return указатель на экземпляр класса видеопроигрывателя.\
В случае ошибки возвращается пустой указатель. */
IVideoPlayerPtr CreateVideoPlayer();

#endif  // VIDEOPLAYER_H
