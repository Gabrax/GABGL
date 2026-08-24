// Shared linear-blend skinning. Backend adapters only provide the spelling of
// vector/matrix types and access to their bone-matrix storage.
#ifndef GABGL_MATRIX4
#error GABGL_MATRIX4 must be defined before including skinning.slang
#endif
#ifndef GABGL_BONE_MATRIX
#error GABGL_BONE_MATRIX must be defined before including skinning.slang
#endif

GABGL_MATRIX4 GabglSkinMatrix(
  GABGL_INT4 boneIds,
  GABGL_FLOAT4 weights,
  int boneBaseIndex,
  bool animated)
{
  if (!animated)
    return GABGL_MATRIX4(
      1,0,0,0,
      0,1,0,0,
      0,0,1,0,
      0,0,0,1);

  GABGL_MATRIX4 skin = GABGL_MATRIX4(
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0);
  float totalWeight = 0.0;
  for (int influence = 0; influence < 4; ++influence)
  {
    const int boneId = boneIds[influence];
    const float weight = weights[influence];
    if (boneId < 0 || boneId >= 100 || weight <= 0.0)
      continue;
    skin += GABGL_BONE_MATRIX(boneBaseIndex + boneId) * weight;
    totalWeight += weight;
  }
  if (totalWeight > 0.00001)
    return skin / totalWeight;
  return GABGL_MATRIX4(
    1,0,0,0,
    0,1,0,0,
    0,0,1,0,
    0,0,0,1);
}

#undef GABGL_MATRIX4
#undef GABGL_INT4
#undef GABGL_FLOAT4
#undef GABGL_BONE_MATRIX
