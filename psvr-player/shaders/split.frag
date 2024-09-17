#version 330 core

out vec4 color;
in vec2 scene_pos;
uniform sampler2D image;
uniform int param;

void main()
{
  vec2 image_pos;
  image_pos.x = scene_pos.x + 1.0f;
  if (param != 0) {
    image_pos.y = scene_pos.y / 2.0 + 0.5f;
  } else {
    image_pos.y = scene_pos.y / -2.0 + 0.5f;
  }
  if (image_pos.x > 1.0f) {
    color = vec4(0, 0, 0, 0);
    return;
  }
  color = texture(image, image_pos);
}
