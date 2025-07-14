#type VERTEX
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 Normal;   
layout (location = 2) in vec2 TexCoord; 
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;
layout (location = 7) in mat4 instanceMatrix;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
	mat4 OrtoProjection;
	mat4 NonRotViewProjection;
  vec3 CameraPos;
};

layout(std430, binding = 5) buffer ModelTransforms { mat4 transforms[]; };
layout(std430, binding = 6) buffer MeshToTransformMap { int meshToTransform[]; };

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
layout(std430, binding = 9) buffer FinalBoneMatrices { mat4 boneMatrices[]; };
layout(std430, binding = 10) buffer ModelIsAnimated  { int modelIsAnimated[]; };

uniform bool isInstanced;

void main()
{
  int transformIndex = meshToTransform[gl_DrawID];
  bool isAnimated = (modelIsAnimated[transformIndex] == 1);
  mat4 modelMat = isInstanced ? instanceMatrix : transforms[transformIndex];

  if (isAnimated)
  {
    int boneBaseIndex = transformIndex * MAX_BONES;
    vec4 totalPosition = vec4(0.0f);
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
      if (boneIds[i] == -1) continue;
      if (boneIds[i] >= MAX_BONES)
      {
          totalPosition = vec4(aPos, 1.0f);
          break;
      }
      vec4 localPosition = boneMatrices[boneBaseIndex + boneIds[i]] * vec4(aPos, 1.0f);
      totalPosition += localPosition * weights[i];
    }

    gl_Position = ViewProjection * modelMat * totalPosition;
  }
  else
  {
    gl_Position = ViewProjection * modelMat * vec4(aPos, 1.0f);
  }
}

#type FRAGMENT
#version 330 core

void main()
{

}
