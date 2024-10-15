#version 330 core

layout (location = 0) in vec3 position;
uniform mat4 transformation; // Матрица трансформации. Содержит трансляцию, поворот и перспективу
out vec4 scene_pos;

void main()
{
  scene_pos = vec4(position.x, position.y, position.z, 1.0);
  gl_Position = scene_pos * transformation;
}
