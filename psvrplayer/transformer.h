#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "framepool.h"
#include "play_screen.h"

class IHelmet;

enum TransformerScheme {
  kSingleImage,  // Одно целиковое изображение. Показывается на два глаза без
                 // объёма
  kLeftRight180  // Изображение из двух частей: левая половина на левый глаз,
                 // правая половина на правый глаз. Показывается в полусфере 180
                 // градусов
};

/*! Класс для трансформации изображения */
class Transformer {
 public:
  virtual ~Transformer() {}

  // TODO ???
  virtual void SetImage(Frame&& frame) = 0;

  // TODO ???
  virtual void SetEyeSwap(bool swap) = 0;

  // TODO ???
  virtual void SetViewPoint(float x_disp, float y_disp) = 0;
};

// TODO Сделать shared_ptr
Transformer* CreateTransformer(TransformerScheme scheme, IPlayScreenPtr screen,
    std::shared_ptr<IHelmet> helmet);

#endif  // TRANSFORMER_H
