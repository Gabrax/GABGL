#type VERTEX
#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 Normal;   
layout (location = 2) in vec2 TexCoord; 
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;
layout (location = 7) in mat4 instanceMatrix;

uniform mat4 u_LightViewProjection;     
uniform mat4 u_ModelTransform;   

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

uniform bool isAnimated;
uniform bool isInstanced;

out vec3 WorldPos;

void main()
{
  mat4 modelMat = isInstanced ? instanceMatrix : u_ModelTransform;

  if (isAnimated)
  {
    vec4 totalPosition = vec4(0.0);
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
      if (boneIds[i] == -1) continue;
      if (boneIds[i] >= MAX_BONES)
      {
          totalPosition = vec4(aPos, 1.0f);
          break;
      }
      vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos, 1.0f);
      totalPosition += localPosition * weights[i];
    }

    WorldPos = (modelMat * vec4(aPos, 1.0f)).xyz;
    gl_Position = u_LightViewProjection * modelMat * totalPosition;
  }
  else
  {
    WorldPos = (modelMat * vec4(aPos, 1.0f)).xyz;
    gl_Position = u_LightViewProjection * modelMat * vec4(aPos, 1.0f);
  }
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
