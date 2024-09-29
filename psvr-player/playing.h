#ifndef PLAYING_H
#define PLAYING_H

#include <memory>

class IVideoPlayer;
class Transformer;

/*! Обработчик клавитурных комбинация для управления воспроизведением */
void KeyProcessor(int key, int scancode, int action, int mods,
    std::shared_ptr<IVideoPlayer> player);

void MouseProcessor(double x_pos, double y_pos, Transformer* transformer);

#endif  // PLAYING_H