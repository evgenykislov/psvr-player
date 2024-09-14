#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include <memory>
#include <string>


class IPlayScreen {
 public:
  virtual ~IPlayScreen() {}

  /*! Запустить цикл обработки окна. Это блокирующий вызов. Запуск выполняется
  только в основном потоке */
  virtual void Run() = 0;

  /*! Установить функцию-фильтр для обработки нажатий кнопок на клавиатуре */
  virtual void SetKeyboardFilter() = 0;

  /*! Сделать окно как текущее в вызываемом потоке */
  virtual void MakeScreenCurrent() = 0;

  virtual void DisplayBuffer() = 0;

  // virtual void DisplayTexture(unsigned int );

};

using IPlayScreenPtr = std::shared_ptr<IPlayScreen>;

/*! Создать окно для проигрывания. Запуск выполняется
только в основном потоке
\return указатель на новое окно. При ошибке возвращается nullptr. */
IPlayScreenPtr CreatePlayScreen(std::string screen);

#endif
