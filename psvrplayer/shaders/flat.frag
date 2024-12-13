#version 330 core

#define M_PI 3.1415926535897932384626433832795

out vec4 color;
in vec4 scene_pos;
uniform sampler2D image;

void main()
{
  vec4 pos = scene_pos;
  float x_pos;
  float y_pos;
  float scale = 1.0; // Масштаб против угла обзора в 90 гр

  if (scene_pos.z >= -0.001f) {
    // Вид за спиной и боковая рамка
    color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  x_pos = scene_pos.x / -scene_pos.z * scale / 2.0f + 0.5f;
  y_pos = scene_pos.y / -scene_pos.z * scale / 2.0f + 0.5f;

  if (x_pos < 0.0f || x_pos > 1.0f || y_pos < 0.0f || y_pos > 1.0f) {
    // Выход за границы кадра
    color = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    return;
  }

  color = texture(image, vec2(x_pos , y_pos));
}
