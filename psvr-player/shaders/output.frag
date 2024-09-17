#version 330 core

out vec4 color;
in vec2 TexCoord;
uniform sampler2D left_image;
uniform sampler2D right_image;

void main()
{
  if (TexCoord.x < 0.5f) {
    color = texture(left_image, vec2(TexCoord.x, TexCoord.y));
  } else {
    color = texture(right_image, vec2(TexCoord.x - 0.5f, TexCoord.y));
  }
}
