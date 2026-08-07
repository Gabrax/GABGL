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
uniform bool u_PS1Effect;

layout(std140, binding = 1) uniform Resolution
{
  vec2 resolution;
};

const float bayer4x4[16] = float[](
   0.0,  8.0,  2.0, 10.0,
  12.0,  4.0, 14.0,  6.0,
   3.0, 11.0,  1.0,  9.0,
  15.0,  7.0, 13.0,  5.0
);

void main()
{
  if (!u_PS1Effect)
  {
    FragColor = texture(u_Texture, v_TexCoord);
    return;
  }

  // Render the scene on a 240-line virtual display while retaining square
  // pixels on widescreen outputs.
  float aspect = resolution.x / max(resolution.y, 1.0);
  vec2 virtualResolution = vec2(max(floor(240.0 * aspect + 0.5), 1.0), 240.0);
  vec2 virtualPixel = floor(v_TexCoord * virtualResolution);
  vec2 snappedUV = (virtualPixel + 0.5) / virtualResolution;

  vec4 source = texture(u_Texture, snappedUV);

  // The original PlayStation used 5 bits per color channel and ordered
  // dithering to hide the resulting color bands.
  ivec2 ditherCoord = ivec2(virtualPixel) & ivec2(3);
  int ditherIndex = ditherCoord.x + ditherCoord.y * 4;
  float threshold = (bayer4x4[ditherIndex] + 0.5) / 16.0;
  vec3 ps1Color = floor(clamp(source.rgb, 0.0, 1.0) * 31.0 + threshold) / 31.0;

  FragColor = vec4(ps1Color, source.a);
}
