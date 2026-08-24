#ifndef GABGL_FLOAT2
#error "Define GABGL_FLOAT2 before including ps1_postprocess.slang"
#endif
#ifndef GABGL_FLOAT3
#error "Define GABGL_FLOAT3 before including ps1_postprocess.slang"
#endif
#ifndef GABGL_INT2
#error "Define GABGL_INT2 before including ps1_postprocess.slang"
#endif
#ifndef GABGL_SATURATE
#error "Define GABGL_SATURATE(value) before including ps1_postprocess.slang"
#endif

float GabglBayerValue(int index)
{
  switch (index)
  {
    case 0:  return 0.0;
    case 1:  return 8.0;
    case 2:  return 2.0;
    case 3:  return 10.0;
    case 4:  return 12.0;
    case 5:  return 4.0;
    case 6:  return 14.0;
    case 7:  return 6.0;
    case 8:  return 3.0;
    case 9:  return 11.0;
    case 10: return 1.0;
    case 11: return 9.0;
    case 12: return 15.0;
    case 13: return 7.0;
    case 14: return 13.0;
    default: return 5.0;
  }
}

float GabglBayerThreshold(GABGL_INT2 pixel)
{
  GABGL_INT2 ditherCoord = pixel & GABGL_INT2(3, 3);
  int ditherIndex = ditherCoord.x + ditherCoord.y * 4;
  return (GabglBayerValue(ditherIndex) + 0.5) / 16.0;
}

GABGL_FLOAT2 GabglPS1SampleUV(
  GABGL_FLOAT2 uv,
  GABGL_FLOAT2 resolution,
  float requestedVirtualHeight,
  out GABGL_FLOAT2 virtualPixel)
{
  resolution = max(resolution, GABGL_FLOAT2(1.0, 1.0));
  float virtualHeight = max(requestedVirtualHeight, 1.0);
  float aspect = resolution.x / resolution.y;
  GABGL_FLOAT2 virtualResolution = GABGL_FLOAT2(
    max(floor(virtualHeight * aspect + 0.5), 1.0), virtualHeight);
  virtualPixel = floor(uv * virtualResolution);
  return (virtualPixel + 0.5) / virtualResolution;
}

GABGL_FLOAT3 GabglPS1Quantize(
  GABGL_FLOAT3 color,
  GABGL_FLOAT2 virtualPixel,
  float requestedColorLevels)
{
  float colorLevels = max(requestedColorLevels, 1.0);
  float threshold = GabglBayerThreshold(GABGL_INT2(virtualPixel));
  return floor(GABGL_SATURATE(color) * colorLevels + threshold) / colorLevels;
}

#undef GABGL_FLOAT2
#undef GABGL_FLOAT3
#undef GABGL_INT2
#undef GABGL_SATURATE
