#type VERTEX
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
  TexCoords = aTexCoords;
  gl_Position = vec4(aPos, 1.0);
}

#type FRAGMENT
#version 330 core

layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform float u_Exposure;
uniform float u_BloomStrength;
uniform float u_Gamma;
uniform bool u_BloomEnabled;

// ACES Tone Mapping
vec3 toneMappingACES(vec3 color)
{
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;

  return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

float gammaCorrection(float value)
{
  return pow(value, 1.0 / max(u_Gamma, 0.001));
}

vec3 gammaCorrection(vec3 value)
{
  return pow(value, vec3(1.0 / max(u_Gamma, 0.001)));
}

void main()
{
  vec3 hdrColor = texture(scene, TexCoords).rgb;
  vec3 bloomColor = u_BloomEnabled ? texture(bloomBlur, TexCoords).rgb : vec3(0.0);

  vec3 result = hdrColor + bloomColor * u_BloomStrength;
  result = toneMappingACES(result * u_Exposure);
  result = gammaCorrection(result);
  FragColor = vec4(result, 1.0);
}
