#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include "play_screen.h"

/*! Класс для трансформации изображения */
class Transformer {
 public:
  virtual ~Transformer() {}

  virtual void SetImage(int width, int height, int align_width, const void* data) = 0;
};

Transformer* CreateTransformer(IPlayScreenPtr screen);


#endif // TRANSFORMER_H
