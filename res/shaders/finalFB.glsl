#type VERTEX
#version 410 core

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 0) out vec2 v_TexCoord;

void main()
{
  v_TexCoord = a_TexCoord;
  gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type FRAGMENT
#version 420 core

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 FragColor;

uniform sampler2D u_Texture;

layout(std140, binding = 2) uniform Resolution
{
  vec2 resolution;
};

void main()
{
  vec2 u_Resolution = resolution;
  int u_PixelSize = 5;

  vec2 pixelCoords = v_TexCoord * u_Resolution;

  float px = float(u_PixelSize);
  vec2 snapped = floor(pixelCoords / px) * px + px * 0.5;

  vec2 uv = snapped / u_Resolution;

  // FragColor = texture(u_Texture, v_TexCoord);
  FragColor = texture(u_Texture, uv);
}
