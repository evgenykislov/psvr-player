#version 330 core

out vec4 color;
in vec2 scene_pos; //!< Позиция пикселя в сцене. Диапазон x=-1..+1; y=-1..+1
uniform sampler2D image;
uniform int part_index;
uniform float image_width; //!< Реальная ширина изобаржения (в долях) относительно полной

void main()
{
  vec2 image_pos = vec2(0.0f, 0.0f);

  if (part_index == 0) {
    // Левая половина изображения
    image_pos.x = scene_pos.x / 4.0 + 0.25f;
    image_pos.y = 0.5f - scene_pos.y / 2.0f;
  } else if (part_index == 1) {
    // Правая половина изображения
    image_pos.x = scene_pos.x / 4.0 + 0.75f;
    image_pos.y = 0.5f - scene_pos.y / 2.0f;
  } else if (part_index == 100) {
    // Изображение целиком
    image_pos.x = scene_pos.x / 2.0 + 0.5f;
    image_pos.y = 0.5f - scene_pos.y / 2.0f;
  }
  if (image_pos.x > 1.0f) {
    color = vec4(0, 0, 0, 0);
    return;
  }
  image_pos.x *= image_width;
  color = texture(image, image_pos);
}
