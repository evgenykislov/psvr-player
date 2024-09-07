#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <memory>


/*! Интерфейс для работы с проигрывателем видеофайлов */
class IVideoPlayer {

};


/*! Функция создания экземпляра видеопроигрывателя
\return указатель на экземпляр класса видеопроигрывателя.\
В случае ошибки возвращается пустой указатель. */
std::unique_ptr<IVideoPlayer> CreateVideoPlayer();

#endif // VIDEOPLAYER_H
