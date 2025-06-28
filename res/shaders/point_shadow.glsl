#type VERTEX
#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 Normal;

uniform mat4 gWVP;
uniform mat4 gWorld;

out vec3 WorldPos;

void main()
{
  vec4 Pos4 = vec4(aPos,1.0f);
  gl_Position = gWVP * Pos4;
  WorldPos = (gWorld * Pos4).xyz;
}

#type FRAGMENT
#version 330 core

in vec3 WorldPos;

uniform vec3 gLightWorldPos;

out float LightToPixelDistance;

void main()
{
  vec3 LightToVertex = WorldPos - gLightWorldPos;
  LightToPixelDistance = length(LightToVertex);
}
