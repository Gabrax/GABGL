#api OPENGL
#type VERTEX
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 Normal;   
layout (location = 2) in vec2 TexCoord; 
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;

uniform mat4 u_LightSpaceMatrix;

layout(std430, binding = 5) buffer ModelTransforms { mat4 transforms[]; };
layout(std430, binding = 6) buffer MeshToTransformMap { int meshToTransform[]; };

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
layout(std430, binding = 9) buffer FinalBoneMatrices { mat4 boneMatrices[]; };
layout(std430, binding = 10) buffer ModelIsAnimated  { int modelIsAnimated[]; };
layout(std430, binding = 13) readonly buffer InstanceTransforms { mat4 instanceTransforms[]; };

#define GABGL_MATRIX4 mat4
#define GABGL_INT4 ivec4
#define GABGL_FLOAT4 vec4
#define GABGL_BONE_MATRIX(index) boneMatrices[index]
#include "common/skinning.slang"

void main()
{
  int transformIndex = meshToTransform[gl_DrawID];
  bool isAnimated = (modelIsAnimated[transformIndex] == 1);
  mat4 modelMat = instanceTransforms[gl_BaseInstance + gl_InstanceID];

  mat4 skin = GabglSkinMatrix(
    boneIds, weights, transformIndex * MAX_BONES, isAnimated);
  gl_Position = u_LightSpaceMatrix * modelMat * skin * vec4(aPos, 1.0f);
}

#type FRAGMENT
#version 330 core

void main()
{

}

#api DX12
#define MAX_BONES 100
cbuffer ShadowConstants : register(b0)
{
  float4x4 LightViewProjection;
  float4x4 Model;
  float4x4 Bones[MAX_BONES];
  float4 Animated;
};

#define GABGL_MATRIX4 float4x4
#define GABGL_INT4 int4
#define GABGL_FLOAT4 float4
#define GABGL_BONE_MATRIX(index) Bones[index]
#include "common/skinning.slang"
struct VSInput { float3 position : POSITION; int4 boneIds : BONEIDS; float4 weights : BONEWEIGHTS; };
float4 VSMain(VSInput input) : SV_POSITION
{
  const float4x4 skin = GabglSkinMatrix(
    input.boneIds, input.weights, 0, Animated.x > 0.5f);
  return mul(LightViewProjection, mul(Model, mul(skin, float4(input.position, 1.0f))));
}
