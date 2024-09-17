#version 330 core

out vec4 color;
in vec2 scene_pos;
uniform sampler2D image;

void main()
{
  vec2 image_pos;
  image_pos.x = scene_pos.x / 2.0 + 0.5f;
  image_pos.y = scene_pos.y / -2.0 + 0.5f;
  color = texture(image, image_pos);
}
