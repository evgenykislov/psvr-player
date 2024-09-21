#version 330 core

out vec4 color;
in vec4 scene_pos;
uniform sampler2D image;

void main()
{
  vec4 pos = scene_pos;

  if (pos.z > 0.99 && pos.z < 1.01) {
    color = vec4(0, 0, 1, 1);
    return;
  }
  if (pos.z < -0.99 && pos.z > -1.01) {
    color = vec4(0, 0, 0.5, 1);
    return;
  }

  if (pos.x > 0.99 && pos.x < 1.01) {
    color = vec4(0, 1, 0, 1);
    return;
  }
  if (pos.x < -0.99 && pos.x > -1.01) {
    color = vec4(0, 0.5, 0, 1);
    return;
  }

  if (pos.y > 0.99 && pos.y < 1.01) {
    color = vec4(1, 0, 0, 1);
    return;
  }
  if (pos.y < -0.99 && pos.y > -1.01) {
    color = vec4(0.5, 0, 0, 1);
    return;
  }

  color = vec4(0, 0, 0, 0);
}
