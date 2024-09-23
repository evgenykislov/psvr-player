#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include <functional>
#include <memory>
#include <string>


class IPlayScreen {
 public:
  virtual ~IPlayScreen() {}

  /*! Запустить цикл обработки окна. Это блокирующий вызов. Запуск выполняется
  только в основном потоке */
  virtual void Run() = 0;

  /*! Установить функцию-фильтр для обработки нажатий кнопок на клавиатуре */
  virtual void SetKeyboardFilter(
      std::function<void(int, int, int, int)> fn) = 0;

  // TODO ???
  virtual void SetMouseEvent(std::function<void(double, double)> fn) = 0;

  /*! Сделать окно как текущее в вызываемом потоке */
  virtual void MakeScreenCurrent() = 0;

  virtual void DisplayBuffer() = 0;

  virtual void GetFrameSize(int& width, int& height) = 0;
  // virtual void DisplayTexture(unsigned int );
};

using IPlayScreenPtr = std::shared_ptr<IPlayScreen>;

/*! Создать окно для проигрывания. Запуск выполняется
только в основном потоке
\return указатель на новое окно. При ошибке возвращается nullptr. */
IPlayScreenPtr CreatePlayScreen(std::string screen);

#endif
