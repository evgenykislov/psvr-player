#version 330 core

#define M_PI 3.1415926535897932384626433832795

out vec4 color;
in vec4 scene_pos;
uniform sampler2D image;

void main()
{
  vec4 pos = scene_pos;
  float x_angle;
  float y_angle;
  float l_hor; //!< Расстояние до отображаемой точки по горизонтали

  if (scene_pos.z >= 0.0f) {
    // Вид за спиной
    color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  if (scene_pos.x < scene_pos.z) {
    // Левая часть. Ось x не равна 0
    x_angle = atan(scene_pos.z / scene_pos.x);
  } else if (scene_pos.x > -scene_pos.z) {
    // Правая часть. Ось x не равна 0
    x_angle = M_PI - atan(-scene_pos.z / scene_pos.x);
  } else {
    // Центральная часть. Ось z не равна 0
    x_angle = M_PI / 2.0f + atan(scene_pos.x / -scene_pos.z);
  }

  l_hor = sqrt(scene_pos.x * scene_pos.x + scene_pos.z * scene_pos.z);
  y_angle = M_PI / 2.0f + atan(scene_pos.y / l_hor);

  color = texture(image, vec2(x_angle / M_PI , y_angle / M_PI));
}
