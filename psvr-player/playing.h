#ifndef PLAYING_H
#define PLAYING_H

#include <memory>

class IVideoPlayer;

/*! Обработчик клавитурных комбинация для управления воспроизведением */
void KeyProcessor(int key, int scancode, int action, int mods,
    std::shared_ptr<IVideoPlayer> player);

#endif  // PLAYING_H
