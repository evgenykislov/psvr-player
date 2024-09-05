#ifndef PLAY_SCREEN_H
#define PLAY_SCREEN_H

#include <string>

class IPlayScreen {

};

/*! Создать окно для проигрывания */
IPlayScreen* CreatePlayScreen(std::string screen);

#endif
