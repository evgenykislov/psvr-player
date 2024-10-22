#version 330 core

out vec4 color;
in vec2 screen_pos; // Позиция тикселя на экране x = 0 .. 1 (направо); y = 0 .. 1 (вверх);
uniform sampler2D left_image;
uniform sampler2D right_image;
// Сведение каждого глазного изображения в центр в долях. Т.е. если параметр
// равен 0.5, то центральная точка изображения будет отображаться на стыке
// изображений
uniform float eyes_correction;


#define DEBUG_DISTORSION


const float kScreenWidth2Height = 960.0f / 1080.0f;


#ifdef DEBUG_DISTORSION

float kDistorsion0 = 1.00; // Len = 0. It's center of screen. k = 1.0
float kDistorsion1 = 1.00; // Len = 0.250. It's first square center of edge. k near 1.0
float kDistorsion2 = 1.02; // Len = 0.354. It's first square corner
float kDistorsion3 = 1.06; // Len = 0.500. Second square edge center
float kDistorsion4 = 1.08; // Len = 0.600. Second square corner and third square edge center
float kDistorsion5 = 1.14; // Len = 0.750. Third square corner and forth square edge center
float kDistorsion6 = 1.23; // Len = 0.900. 1/4 and 3/4 of forth square edge
float kDistorsion7 = 1.36; // Len = 1.050. Forth square corner
float kDistorsion8 = 1.55; // Len = 1.200. ???
float kDistorsionMaxDist = 1.50;

vec2 FixDistorsion(vec2 pos) {
  // Отладочная версия
  vec2 cpos;
  cpos.x = pos.x;
  cpos.y = pos.y;
  float len = length(cpos);
  if (len > kDistorsionMaxDist) { return vec2(100.0f, 100.0f); } // Very far point
  float k = 1.0;
  if (len <= 0.250) {
    k = kDistorsion0 + (kDistorsion1 - kDistorsion0) / 0.250 * len;
  } else if (len <= 0.354) {
    k = kDistorsion1 + (kDistorsion2 - kDistorsion1) / 0.104 * (len - 0.250);
  } else if (len <= 0.500) {
    k = kDistorsion2 + (kDistorsion3 - kDistorsion2) / 0.146 * (len - 0.354);
  } else if (len <= 0.600) {
    k = kDistorsion3 + (kDistorsion4 - kDistorsion3) / 0.100 * (len - 0.500);
  } else if (len <= 0.750) {
    k = kDistorsion4 + (kDistorsion5 - kDistorsion4) / 0.150 * (len - 0.600);
  } else if (len <= 0.900) {
    k = kDistorsion5 + (kDistorsion6 - kDistorsion5) / 0.150 * (len - 0.750);
  } else if (len <= 1.050) {
    k = kDistorsion6 + (kDistorsion7 - kDistorsion6) / 0.150 * (len - 0.900);
  } else {
    k = kDistorsion7 + (kDistorsion8 - kDistorsion7) / 0.150 * (len - 1.050);
  }

  pos.x *= k;
  pos.y *= k;
  return pos;
}

#else

vec2 FixDistorsion(vec2 pos) {
  vec2 cpos;
  cpos.x = pos.x;
  cpos.y = pos.y;
  float len = length(cpos);

  if (len > 1.5) { return pos; }
  float km1 = -0.02328336 * len * len * len + 0.33334678 * len * len -
      0.10098184 * len + 1.00274654;
  float k = 1.0 / km1;

  pos.x *= k;
  pos.y *= k;
  return pos;
}

#endif



vec4 GetTextureColor(vec2 frame_pos, int eye_index) {
  vec2 pos = frame_pos;
  vec2 center_pos = vec2(frame_pos.x * 2.0f - 1.0, frame_pos.y * 2.0f - 1.0);
  vec2 fix_pos = FixDistorsion(center_pos);
  if (fix_pos.x < -1.0 || fix_pos.x > 1.0 || fix_pos.y < -1.0 || fix_pos.y > 1.0) {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }

  vec2 tx_pos = vec2(fix_pos.x / 2.0 + 0.5, fix_pos.y / 2.0f + 0.5);
  vec2 move_pos = tx_pos;
  if (eye_index == 0) {
    move_pos.x -= eyes_correction;
  } else {
    move_pos.x += eyes_correction;
  }
  if (move_pos.x < 0.0 || move_pos.x > 1.0) {
    return vec4(0.0, 0.0, 0.0, 0.0);
  }

  if (eye_index == 0) {
    return texture(left_image, move_pos);
  }
  return texture(right_image, move_pos);
}



void main()
{
  float xpos;
  float ypos;
  int eye_index;

  float bd = 0.5f - kScreenWidth2Height / 2.0f; // Нижняя граница изображения
  float bu = 0.5f + kScreenWidth2Height / 2.0f; // Верхняя граница изображения
  if (screen_pos.y < bd || screen_pos.y > bu) {
    color = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    return;
  }

  if (screen_pos.x < 0.5f) {
    xpos = screen_pos.x * 2.0f;
    eye_index = 0;
  } else {
    xpos = screen_pos.x * 2.0f - 1.0f;
    eye_index = 1;
  }
  ypos = (screen_pos.y - bd) / kScreenWidth2Height;

  color = GetTextureColor(vec2(xpos, ypos), eye_index);
}
