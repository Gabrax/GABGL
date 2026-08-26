#type COMPUTE
#version 460 core

layout(r32f, binding = 0) writeonly uniform image2D u_Destination;
uniform sampler2D u_Source;
uniform int u_SourceMip;
uniform bool u_CopyDepth;
uniform vec2 u_DestinationSize;

[numthreads(8, 8, 1)]
void main()
{
  ivec2 destinationSize = ivec2(u_DestinationSize);
  ivec2 destinationPixel = ivec2(gl_GlobalInvocationID.xy);
  if (any(greaterThanEqual(destinationPixel, destinationSize)))
    return;

  float maximumDepth;
  if (u_CopyDepth)
  {
    maximumDepth = texelFetch(u_Source, destinationPixel, 0).r;
  }
  else
  {
    ivec2 sourceSize = textureSize(u_Source, u_SourceMip);
    ivec2 sourcePixel = destinationPixel * 2;
    maximumDepth = 0.0;
    for (int y = 0; y < 2; ++y)
      for (int x = 0; x < 2; ++x)
        maximumDepth = max(maximumDepth, texelFetch(
          u_Source, min(sourcePixel + ivec2(x, y), sourceSize - 1), u_SourceMip).r);
  }
  imageStore(u_Destination, destinationPixel, vec4(maximumDepth));
}
