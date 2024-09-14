#version 330 core

out vec4 color;
in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
  if (TexCoord.x < 0.1 || TexCoord.x > 0.9f || TexCoord.y < 0.1f || TexCoord.y > 0.9f)
  { color = vec4(1.0f, 0, 0, 1.0f); return; }
//  color = vec4(TexCoord.y, TexCoord.x, 1.0f, 1.0f);
//  return;
  vec4 tc = texture(ourTexture, TexCoord);
  color = tc;
  return;
  color = color / 2 + tc;
  color = tc;
//  color = vec4(1.0f, 0.5f, 0.2f, 0.5f);
}
