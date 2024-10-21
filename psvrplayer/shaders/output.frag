#version 330 core

out vec4 color;
in vec2 screen_pos; // Позиция тикселя на экране x = 0 .. 1 (направо); y = 0 .. 1 (вверх);
uniform sampler2D left_image;
uniform sampler2D right_image;
// Сведение каждого глазного изображения в центр в долях. Т.е. если параметр
// равен 0.5, то центральная точка изображения будет отображаться на стыке
// изображений
uniform float eyes_correction;

const float kScreenWidth2Height = 960.0f / 1080.0f;

void main()
{
  float xpos;
  float ypos;
  int tx_index;

  float bd = 0.5f - kScreenWidth2Height / 2.0f; // Нижняя граница изображения
  float bu = 0.5f + kScreenWidth2Height / 2.0f; // Верхняя граница изображения
  if (screen_pos.y < bd || screen_pos.y > bu) {
    color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  if (screen_pos.x < 0.5f) {
    xpos = screen_pos.x * 2.0f;
    tx_index = 0;
  } else {
    xpos = screen_pos.x * 2.0f - 1.0f;
    tx_index = 1;
  }
  ypos = (screen_pos.y - bd) / kScreenWidth2Height;

  // Eyes eyes_correction
  if (tx_index == 0) {
    xpos -= eyes_correction;
  } else {
    xpos += eyes_correction;
  }
  if (xpos > 1.0f || xpos < 0.0f) {
    color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  if (tx_index == 0) {
    color = texture(left_image, vec2(xpos, ypos));
  } else {
    color = texture(right_image, vec2(xpos, ypos));
  }
}
