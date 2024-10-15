#version 330 core

out vec4 color;
in vec2 screen_pos; // Позиция тикселя на экране x = 0 .. 1 (направо); y = 0 .. 1 (вверх);
uniform sampler2D left_image;
uniform sampler2D right_image;

const float kScreenWidth2Height = 960.0f / 1080.0f;

void main()
{
  float bd = 0.5f - kScreenWidth2Height / 2.0f;
  float bu = 0.5f + kScreenWidth2Height / 2.0f;
  if (screen_pos.y < bd || screen_pos.y > bu) {
    color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  if (screen_pos.x < 0.5f) {
    color = texture(left_image, vec2(screen_pos.x * 2.0f,
        (screen_pos.y - bd) / kScreenWidth2Height));
  } else {
    color = texture(right_image, vec2(screen_pos.x * 2.0f - 1.0f,
        (screen_pos.y - bd) / kScreenWidth2Height));
  }
}
