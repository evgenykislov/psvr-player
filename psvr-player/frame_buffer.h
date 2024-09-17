#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

/*! Структура для описания кадрового буфера. Структура не создаётся
автоматически, т.к. она должна создаваться в определённом контексте, с текущим
окном и т.д. Для создания и удаления используйте функции CreateFrameBuffer и
DeleteFrameBuffer */
struct FrameBuffer {
  unsigned int buffer;  //!< Объект кадрового буфера, его номер в OpenGL
  unsigned int texture;  //!< ОБъект текстуры кадрового буфера (номер)

  static const unsigned int texture_size = 1920;
};

/*! Создаём фреймбуфер ??? */
bool CreateFrameBuffer(FrameBuffer& buffer);

// TODO ???
void DeleteFrameBuffer(FrameBuffer& buffer);

#endif  // FRAME_BUFFER_H
