#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "framepool.h"
#include "play_screen.h"

/*! Класс для трансформации изображения */
class Transformer {
 public:
  virtual ~Transformer() {}

  // TODO ???
  virtual void SetImage(Frame&& frame) = 0;

  // TODO ???
  virtual void SetEyeSwap(bool swap) = 0;
};

Transformer* CreateTransformer(IPlayScreenPtr screen);

#endif  // TRANSFORMER_H
