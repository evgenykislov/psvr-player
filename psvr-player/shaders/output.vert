#version 330 core

layout (location = 0) in vec3 position;
out vec2 screen_pos;

void main()
{
  gl_Position = vec4(position.x, position.y, position.z, 1.0);
  screen_pos = position.xy / 2 + vec2(0.5f, 0.5f);
}
