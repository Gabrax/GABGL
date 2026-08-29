#type COMPUTE
#version 460 core

layout(binding = 1) uniform sampler2D gPosition;

layout(std430, binding = 0) readonly buffer LightPositions
{
  vec4 positions[];
};
layout(std430, binding = 2) readonly buffer LightsQuantity
{
  int numLights;
};
layout(std430, binding = 4) readonly buffer LightTypes
{
  int lightTypes[];
};
layout(std430, binding = 21) writeonly buffer TileLightGrid
{
  uvec2 tileLightGrid[];
};
layout(std430, binding = 22) writeonly buffer TileLightIndices
{
  uint tileLightIndices[];
};

uniform int u_TileCountX;

const int TILE_SIZE = 16;
const uint MAX_LIGHTS_PER_TILE = 128u;
const float LOCAL_LIGHT_RADIUS = 20.0;

groupshared vec4 sharedMinimumPositions[256];
groupshared vec4 sharedMaximumPositions[256];
groupshared uint sharedLightCount;
groupshared uint sharedLightIndices[MAX_LIGHTS_PER_TILE];

bool SphereIntersectsAABB(vec3 center, float radius, vec3 minimumPoint,
                          vec3 maximumPoint)
{
  const vec3 closest = clamp(center, minimumPoint, maximumPoint);
  const vec3 offset = center - closest;
  return dot(offset, offset) <= radius * radius;
}

// One workgroup maps directly to one 16x16 tile. Every lane reads one G-buffer
// pixel, then the group reduces those positions to a conservative world-space
// AABB and tests the lights in parallel.
[numthreads(16, 16, 1)]
void main()
{
  const ivec2 tile = ivec2(gl_GlobalInvocationID.xy);
  const ivec2 imageSize = textureSize(gPosition, 0);
  const ivec2 tileCoordinate = ivec2(gl_WorkGroupID.xy);
  const uint tileIndex = uint(tileCoordinate.y * u_TileCountX + tileCoordinate.x);
  const uint localIndex = gl_LocalInvocationIndex;

  const bool insideImage = all(lessThan(tile, imageSize));
  const vec4 positionData = insideImage
    ? texelFetch(gPosition, tile, 0)
    : vec4(0.0);
  const bool validPosition = positionData.a > 0.0;
  sharedMinimumPositions[localIndex] = validPosition
    ? vec4(positionData.xyz, 1.0)
    : vec4(vec3(3.402823e38), 0.0);
  sharedMaximumPositions[localIndex] = validPosition
    ? vec4(positionData.xyz, 1.0)
    : vec4(vec3(-3.402823e38), 0.0);
  if (localIndex == 0u)
    sharedLightCount = 0u;
  memoryBarrierShared();
  barrier();

  for (uint stride = 128u; stride > 0u; stride >>= 1u)
  {
    if (localIndex < stride)
    {
      sharedMinimumPositions[localIndex].xyz = min(
        sharedMinimumPositions[localIndex].xyz,
        sharedMinimumPositions[localIndex + stride].xyz);
      sharedMaximumPositions[localIndex].xyz = max(
        sharedMaximumPositions[localIndex].xyz,
        sharedMaximumPositions[localIndex + stride].xyz);
      sharedMinimumPositions[localIndex].w = max(
        sharedMinimumPositions[localIndex].w,
        sharedMinimumPositions[localIndex + stride].w);
    }
    memoryBarrierShared();
    barrier();
  }

  const bool hasGeometry = sharedMinimumPositions[0].w > 0.0;
  if (hasGeometry)
  {
    for (int lightIndex = int(localIndex); lightIndex < numLights; lightIndex += 256)
    {
      const int type = lightTypes[lightIndex];
      const bool affectsTile = type == 0 || SphereIntersectsAABB(
        positions[lightIndex].xyz, LOCAL_LIGHT_RADIUS,
        sharedMinimumPositions[0].xyz, sharedMaximumPositions[0].xyz);
      if (!affectsTile)
        continue;
      const uint slot = atomicAdd(sharedLightCount, 1u);
      if (slot < MAX_LIGHTS_PER_TILE)
        sharedLightIndices[slot] = uint(lightIndex);
    }
  }
  memoryBarrierShared();
  barrier();

  const uint storedLightCount = min(sharedLightCount, MAX_LIGHTS_PER_TILE);
  if (localIndex < storedLightCount)
    tileLightIndices[tileIndex * MAX_LIGHTS_PER_TILE + localIndex] =
      sharedLightIndices[localIndex];
  if (localIndex == 0u)
    tileLightGrid[tileIndex] = uvec2(
      tileIndex * MAX_LIGHTS_PER_TILE, storedLightCount);
}
