#include "play_screen.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Glfw library includes
#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


class OpenGLScreen: public IPlayScreen {
 public:
  OpenGLScreen(std::string screen);
  virtual ~OpenGLScreen();

 private:
  OpenGLScreen() = delete;
  OpenGLScreen(const OpenGLScreen&) = delete;
  OpenGLScreen(OpenGLScreen&&) = delete;
  OpenGLScreen& operator=(const OpenGLScreen&) = delete;
  OpenGLScreen& operator=(OpenGLScreen&&) = delete;

  GLFWwindow* window_;

  GLFWmonitor* GetMonitor(std::string screen);
  GLFWwindow* CreateWindow(GLFWmonitor* monitor);

};


IPlayScreen* CreatePlayScreen(std::string screen) {
  try {
    return new OpenGLScreen(screen);
  }  catch (std::exception& err) {
    std::cerr << "ERROR: " << err.what() << std::endl;
  }
  return nullptr;
}


OpenGLScreen::OpenGLScreen(std::string screen): window_(nullptr) {
  if (!glfwInit()) {
    throw std::runtime_error("Can't initialize GLFW library");
  }
  try {
    auto mon = GetMonitor(screen);
    if (!mon) { throw std::runtime_error("Can't find specific screen"); }
    window_ = CreateWindow(mon);
    if (!window_) { throw std::runtime_error("Can't create window for screen"); }
    // TODO Debug code
    int width, height;
    glfwGetWindowSize(window_, &width, &height);
    std::cout << "Created window with size: " << width << "x" << height << std::endl;
  } catch (...) {
    glfwTerminate();
    throw;
  }
}


GLFWmonitor* OpenGLScreen::GetMonitor(std::string screen) {
  GLFWmonitor** mons;
  int amount;

  mons = glfwGetMonitors(&amount);
  if (!mons) {
    throw std::runtime_error("ERROR: Can't get screens list");
  }
  for (int i = 0; i < amount; ++i) {
    int x, y;
    glfwGetMonitorPos(mons[i], &x, &y);
    std::stringstream s;
    s << x << "x" << y;
    if (s.str() == screen) {
      return mons[i];
    }
  }

  return nullptr;
}


GLFWwindow* OpenGLScreen::CreateWindow(GLFWmonitor* monitor) {
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
  glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
  return glfwCreateWindow(mode->width, mode->height, "PS VR Player", monitor, NULL);
}


OpenGLScreen::~OpenGLScreen() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  glfwTerminate();
}
