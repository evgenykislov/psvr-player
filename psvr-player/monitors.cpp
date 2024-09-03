#include "monitors.h"

#include <iostream>

#define GLAD_GL_IMPLEMENTATION
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


int PrintMonitors() {
  GLFWmonitor** mons;
  int amount;
  bool res = 0;

  if (!glfwInit()) {
    std::cerr << "Can't initialize GLFW library" << std::endl;
    return 1;
  }
  mons = glfwGetMonitors(&amount);
  if (!mons) {
    std::cerr << "ERROR: Can't get monitors list" << std::endl;
    res = 1;
  } else {
    std::cout << "Detected " << amount << " monitor[s]:" << std::endl;
    for (int i = 0; i < amount; ++i) {
      auto name = glfwGetMonitorName(mons[i]);
      int x, y;
      glfwGetMonitorPos(mons[i], &x, &y);
      if (name) {
        std::cout << "  " << i + 1 << ". " << name << ". Position " << x << "x" << y << std::endl;
      } else {
        std::cout << "  unknown monitor" << std::endl;
      }
    }
  }

  glfwTerminate();
  return res;
}
