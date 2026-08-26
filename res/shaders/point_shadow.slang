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

uniform mat4 u_LightViewProjection;     

layout(std430, binding = 5) buffer ModelTransforms { mat4 transforms[]; };
layout(std430, binding = 6) buffer MeshToTransformMap { int meshToTransform[]; };

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
layout(std430, binding = 9) buffer FinalBoneMatrices { mat4 boneMatrices[]; };
layout(std430, binding = 10) buffer ModelIsAnimated  { int modelIsAnimated[]; };
layout(std430, binding = 13) readonly buffer InstanceTransforms { mat4 instanceTransforms[]; };
layout(std430, binding = 14) readonly buffer DrawRemap { uint compactDrawToOriginal[]; };

uniform bool u_GPUDriven;

#define GABGL_MATRIX4 mat4
#define GABGL_INT4 ivec4
#define GABGL_FLOAT4 vec4
#define GABGL_BONE_MATRIX(index) boneMatrices[index]
#include "common/skinning.slang"

out vec3 WorldPos;

void main()
{
  uint drawID = u_GPUDriven ? compactDrawToOriginal[gl_DrawID] : uint(gl_DrawID);
  int transformIndex = meshToTransform[drawID];
  bool isAnimated = (modelIsAnimated[transformIndex] == 1);
  mat4 modelMat = instanceTransforms[gl_BaseInstance + gl_InstanceID];

  mat4 skin = GabglSkinMatrix(
    boneIds, weights, transformIndex * MAX_BONES, isAnimated);
  vec4 worldPosition = modelMat * skin * vec4(aPos, 1.0f);
  WorldPos = worldPosition.xyz;
  gl_Position = u_LightViewProjection * worldPosition;
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

#api DX12
#define MAX_BONES 100
cbuffer PointShadowConstants : register(b0)
{
  float4x4 LightViewProjection;
  float4x4 Model;
  float4x4 Bones[MAX_BONES];
  float4 LightPositionAndAnimated;
};

#define GABGL_MATRIX4 float4x4
#define GABGL_INT4 int4
#define GABGL_FLOAT4 float4
#define GABGL_BONE_MATRIX(index) Bones[index]
#include "common/skinning.slang"
struct VSInput { float3 position : POSITION; int4 boneIds : BONEIDS; float4 weights : BONEWEIGHTS; };
struct PSInput { float4 position : SV_POSITION; float3 worldPosition : POSITION0; };
PSInput VSMain(VSInput input)
{
  const float4x4 skin = GabglSkinMatrix(
    input.boneIds, input.weights, 0, LightPositionAndAnimated.w > 0.5f);
  const float4 world = mul(Model, mul(skin, float4(input.position, 1.0f)));
  PSInput output;
  output.position = mul(LightViewProjection, world);
  // GLM builds right-handed cubemap views, while Direct3D's cube lookup uses
  // the opposite horizontal face orientation. Match the sampled cube face.
  output.position.x = -output.position.x;
  output.worldPosition = world.xyz;
  return output;
}
float PSMain(PSInput input) : SV_TARGET
{
  return length(input.worldPosition - LightPositionAndAnimated.xyz);
}
