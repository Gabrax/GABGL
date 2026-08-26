#type VERTEX
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 5) in ivec4 boneIds;
layout(location = 6) in vec4 weights;

layout(std140, binding = 0) uniform Camera
{
  mat4 ViewProjection;
  mat4 OrtoProjection;
  mat4 NonRotViewProjection;
  vec3 CameraPos;
};
layout(std140, binding = 1) uniform Resolution { vec2 resolution; };
layout(std430, binding = 6) readonly buffer MeshToTransformMap { int meshToTransform[]; };
layout(std430, binding = 9) readonly buffer FinalBoneMatrices { mat4 boneMatrices[]; };
layout(std430, binding = 10) readonly buffer ModelIsAnimated { int modelIsAnimated[]; };
layout(std430, binding = 13) readonly buffer InstanceTransforms { mat4 instanceTransforms[]; };
layout(std430, binding = 14) readonly buffer DrawRemap { uint compactDrawToOriginal[]; };

#define GABGL_MATRIX4 mat4
#define GABGL_INT4 ivec4
#define GABGL_FLOAT4 vec4
#define GABGL_BONE_MATRIX(index) boneMatrices[index]
#include "common/skinning.slang"

uniform bool u_PS1Enabled;
uniform float u_PS1VirtualHeight;

void main()
{
  uint originalDrawID = compactDrawToOriginal[gl_DrawID];
  int transformIndex = meshToTransform[originalDrawID];
  mat4 modelMatrix = instanceTransforms[gl_BaseInstance + gl_InstanceID];
  mat4 skinMatrix = GabglSkinMatrix(
    boneIds, weights, transformIndex * 100,
    modelIsAnimated[transformIndex] == 1);
  // Keep the position path identical to scene.slang. The prepass depth is
  // reused with GL_LEQUAL, so even a different matrix-association chosen by
  // the compiler can otherwise create moving holes on animated geometry.
  mat4 localToWorld = modelMatrix * skinMatrix;
  vec4 worldPos = localToWorld * vec4(aPos, 1.0);
  vec4 clipPosition = ViewProjection * worldPos;

  if (u_PS1Enabled)
  {
    vec2 outputResolution = max(resolution, vec2(1.0));
    float virtualHeight = max(u_PS1VirtualHeight, 1.0);
    vec2 snapResolution = vec2(
      max(floor(virtualHeight * outputResolution.x / outputResolution.y + 0.5), 1.0),
      virtualHeight);
    float safeW = abs(clipPosition.w) > 0.00001 ? clipPosition.w : 0.00001;
    vec2 ndc = clipPosition.xy / safeW;
    vec2 snappedNdc =
      (floor((ndc * 0.5 + 0.5) * snapResolution + 0.5) / snapResolution) * 2.0 - 1.0;
    clipPosition.xy = snappedNdc * clipPosition.w;
  }
  gl_Position = clipPosition;
}

#type FRAGMENT
#version 460 core
void main() {}
