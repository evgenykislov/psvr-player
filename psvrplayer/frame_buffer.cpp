#include "frame_buffer.h"

#include <iostream>

// clang-format off
// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// clang-format on

bool CreateFrameBuffer(FrameBuffer& buffer) {
  unsigned int fbo;
  glGenFramebuffers(1, &fbo);

  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FrameBuffer::texture_size,
      FrameBuffer::texture_size, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);

  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  buffer.buffer = fbo;
  buffer.texture = texture;
  return true;
}

void DeleteFrameBuffer(FrameBuffer& buffer) {
  glDeleteTextures(1, &buffer.texture);
  buffer.texture = 0;
  glDeleteFramebuffers(1, &buffer.buffer);
  buffer.buffer = 0;
}
