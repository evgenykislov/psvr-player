#ifndef FRAMEPOOL_H
#define FRAMEPOOL_H


#include <memory>
#include <vector>

/*! Описатель одного кадра. Класс содержит данные согласно размерам.
Экземпляры класса не копируются, однако их можно перемещать */
class Frame {
 public:
  /*! Создаём фрейм с указанным максимальным размером. Если при создании
  возникла ошибка, то выбрасывается исключение */
  Frame(int align_width, int align_height);

  ~Frame() {}

  Frame(Frame&& arg);
  Frame& operator=(Frame&& arg);

  /*! Установить внутренний размер кадра. Он может быть меньше максимального
  (align_width, align_height) */
  void SetSize(int width, int height);

  /*! Выдать все установленные размеры. Все аргументы опциональные, если
  результат не нужен, то можно передавать nullptr */
  void GetSizes(int* width, int* height, int* align_width, int* align_height);

  /*! Выдать указатель на сырые хранимые данные. Также возвращается размер этих
  данных в байтах */
  void* GetData(size_t& data_size);

 private:
  Frame() = delete;
  Frame(const Frame&) = delete;
  Frame& operator=(const Frame&) = delete;

  const size_t kPixelSize = 4;

  int width_;
  int height_;
  int align_width_;
  int align_height_;
  std::vector<uint8_t> data_;

  void Move(Frame&& arg);
};


/*! Запросить фрейм. Фрейм может быть создан или взят из предыдущих.
Если фрейм не создан, то выбрасывается исключение
\param align_width максимальная ширина кадра
\param align_height максимальная высота кадра
\return ново-созданный фрейм */
Frame RequestFrame(int align_width, int align_height);

/*! Освободить фрейм frame */
void ReleaseFrame(Frame&& frame);

#endif // FRAMEPOOL_H
