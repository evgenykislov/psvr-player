#version 330 core

out vec4 color;
in vec2 screen_pos; // Позиция тикселя на экране x = 0 .. 1 (направо); y = 0 .. 1 (вверх);
uniform sampler2D left_image;
uniform sampler2D right_image;
// Сведение каждого глазного изображения в центр в долях. Т.е. если параметр
// равен 0.5, то центральная точка изображения будет отображаться на стыке
// изображений
uniform float eyes_correction;

// Включить отладочный код для подбора параметров дистории
// #define DEBUG_DISTORSION

const float kScreenWidth2Height = 960.0f / 1080.0f;

// Максимальная обрабатываемая дистанция для компенсации дисторсии
float kDistorsionMaxDist = 1.300f;

// Функция FixDistorsion принимает на вход координаты пикселя, который нужно
// отобразить, в прямоугольном полу и возвращает координаты пикселя, откуда
// нужно брать цвет
// Координаты задаются в диапазоне x=-1..+1; y=-1..+1

#ifdef DEBUG_DISTORSION

#define kPointAmount 8

// Два массива содержат точки аппроксимации: длина и корректирующий масштабный
// коэффициент. Длины должны идти в возрастающем порядке. Рекомендауется
// начинать с длины 0.0f

float DistorsionLength[kPointAmount] = float[kPointAmount](
    0.000f, 0.240f, 0.420f, 0.660f, 0.880f, 0.960f, 1.040f, 1.300f);
float DistorsionScale[kPointAmount] = float[kPointAmount](
    0.810f, 0.820f, 0.840f, 0.900f, 1.000f, 1.060f, 1.140f, 1.480f);

// Характерные длины
// 0.24f  - ближняя кромка середины стороны 1-го квадрата
// 0.53f  - ближняя кромка середины стороны 2-го квадрата
// 0.76f  - ближняя кромка середины стороны 3-го квадрата
// 0.94f  - ближняя кромка середины стороны 4-го квадрата
// 1.04f  - угол 4-го квадрата


vec2 FixDistorsion(vec2 pos) {
  // Отладочная версия
  vec2 cpos;
  float k = 0.0f;
  cpos.x = pos.x;
  cpos.y = pos.y;
  float len = length(cpos);

  if (len > kDistorsionMaxDist) { return vec2(100.0f, 100.0f); } // Very far point

  for (int i = 1; i < kPointAmount; ++i) {
    if (len > DistorsionLength[i]) { continue; }
    // Нашли нужный интервал (i - 1; i]
    float d = DistorsionLength[i] - DistorsionLength[i - 1];
    if (d <= 0.0001f) {
      k = DistorsionScale[i - 1];
    } else {
      k = (DistorsionScale[i] - DistorsionScale[i - 1]) / d *
          (len - DistorsionLength[i - 1]) + DistorsionScale[i - 1];
    }
    break;
  }

  pos.x *= k;
  pos.y *= k;
  return pos;
}

#else

vec2 FixDistorsion(vec2 pos) {
  const float kBase = 0.81f;
  const float kJam = 0.35f;
  const float kScale = 0.016786f;
  vec2 cpos;
  cpos.x = pos.x;
  cpos.y = pos.y;
  float len = length(cpos);

  if (len > kDistorsionMaxDist) { return vec2(100.0f, 100.0f); } // Very far point
  float k = kScale * (exp(len / kJam) - kBase) + kBase;

  pos.x *= k;
  pos.y *= k;
  return pos;
}

#endif


vec2 GetGreenPos(vec2 red_pos) {
  vec2 res;
  const float kHorizontalBase = 0.4f;
  const float kHorizontalScale = 1.008f;
  const float kVerticalBase = 0.59f;
  const float kVerticalScale = 1.008f;
  res.x = (red_pos.x - kHorizontalBase) * kHorizontalScale + kHorizontalBase;
  res.y = (red_pos.y - kVerticalBase) * kVerticalScale + kVerticalBase;
  return res;
}


vec2 GetBluePos(vec2 red_pos) {
  vec2 res;
  const float kHorizontalBase = 0.4f;
  const float kHorizontalScale = 1.017f;
  const float kVerticalBase = 0.59f;
  const float kVerticalScale = 1.017f;
  res.x = (red_pos.x - kHorizontalBase) * kHorizontalScale + kHorizontalBase;
  res.y = (red_pos.y - kVerticalBase) * kVerticalScale + kVerticalBase;
  return res;
}


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
  vec2 red_pos;
  vec2 green_pos;
  vec2 blue_pos;


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

  red_pos = vec2(xpos, ypos);
  green_pos = GetGreenPos(red_pos);
  blue_pos = GetBluePos(red_pos);

  color.r = GetTextureColor(red_pos, eye_index).r;
  color.g = GetTextureColor(green_pos, eye_index).g;
  color.b = GetTextureColor(blue_pos, eye_index).b;
  color.a = 1.0f;
}
