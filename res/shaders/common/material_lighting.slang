// API-neutral material and lighting math. The including adapter supplies the
// vector spelling and saturation intrinsic used by its source language.
#ifndef GABGL_FLOAT3
#error GABGL_FLOAT3 must be defined before including material_lighting.slang
#endif
#ifndef GABGL_SATURATE
#error GABGL_SATURATE must be defined before including material_lighting.slang
#endif

GABGL_FLOAT3 GabglDecodeNormal(GABGL_FLOAT3 encodedNormal)
{
  return encodedNormal * 2.0 - 1.0;
}

GABGL_FLOAT3 GabglApplyTangentNormal(
  GABGL_FLOAT3 geometricNormal,
  GABGL_FLOAT3 tangent,
  GABGL_FLOAT3 bitangent,
  GABGL_FLOAT3 encodedNormal)
{
  const GABGL_FLOAT3 tangentNormal = GabglDecodeNormal(encodedNormal);
  return normalize(
    tangentNormal.x * tangent +
    tangentNormal.y * bitangent +
    tangentNormal.z * geometricNormal);
}

float GabglMaterialShininess(float specularStrength)
{
  return 8.0 + (128.0 - 8.0) * GABGL_SATURATE(specularStrength);
}

float GabglDistanceAttenuation(float distanceToLight)
{
  return 1.0 / (1.0 + 0.09 * distanceToLight +
    0.032 * distanceToLight * distanceToLight);
}

float GabglSpotIntensity(GABGL_FLOAT3 fromLight, GABGL_FLOAT3 lightDirection)
{
  const float theta = dot(normalize(fromLight), normalize(lightDirection));
  const float innerCutoff = cos(0.2181661565); // 12.5 degrees
  const float outerCutoff = cos(0.3054326191); // 17.5 degrees
  return GABGL_SATURATE(
    (theta - outerCutoff) / max(innerCutoff - outerCutoff, 0.0001));
}

GABGL_FLOAT3 GabglEvaluateLight(
  GABGL_FLOAT3 lightColor,
  GABGL_FLOAT3 albedo,
  GABGL_FLOAT3 specularColor,
  GABGL_FLOAT3 normal,
  GABGL_FLOAT3 viewDirection,
  GABGL_FLOAT3 toLight,
  float shininess,
  float attenuation,
  float intensity,
  float shadow)
{
  const float diffuseStrength = GABGL_SATURATE(dot(normal, toLight));
  const float specularStrength = pow(
    GABGL_SATURATE(dot(viewDirection, reflect(-toLight, normal))), shininess);
  const GABGL_FLOAT3 ambient = lightColor * 0.1 * albedo * attenuation;
  const GABGL_FLOAT3 diffuse =
    lightColor * diffuseStrength * albedo * attenuation * intensity * shadow;
  const GABGL_FLOAT3 specular =
    specularColor * specularStrength * attenuation * intensity * shadow;
  return ambient + diffuse + specular;
}

#undef GABGL_FLOAT3
#undef GABGL_SATURATE
