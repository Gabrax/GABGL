#define MAX_BONES 100
#define MAX_LIGHTS 32

cbuffer SceneConstants : register(b0)
{
  float4x4 ViewProjection;
  float4x4 Model;
  float4x4 LightViewProjection;
  float4x4 Bones[MAX_BONES];
  float4 CameraPosition;
  float4 MaterialFlags;
  float4 LightPositions[MAX_LIGHTS];
  float4 LightDirections[MAX_LIGHTS];
  float4 LightColors[MAX_LIGHTS];
  float4 LightTypes[MAX_LIGHTS];
  uint4 LightCount;
  float4 Resolution;
};

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D SpecularTexture : register(t2);
Texture2D ShadowTexture : register(t3);
TextureCubeArray PointShadowTexture : register(t4);
SamplerState LinearSampler : register(s0);

struct VSInput
{
  float3 position : POSITION;
  float3 normal : NORMAL;
  float2 uv : TEXCOORD;
  float3 tangent : TANGENT;
  float3 bitangent : BITANGENT;
  int4 boneIds : BONEIDS;
  float4 weights : BONEWEIGHTS;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float3 worldPosition : POSITION0;
  float3 normal : NORMAL;
  float3 tangent : TANGENT;
  float3 bitangent : BITANGENT;
  float2 uv : TEXCOORD;
  float4 shadowPosition : TEXCOORD1;
};

float4x4 SkinMatrix(VSInput input)
{
  float4x4 skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  if (CameraPosition.w < 0.5f) return skin;
  skin = float4x4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
  float totalWeight = 0.0f;
  [unroll]
  for (int i = 0; i < 4; ++i)
  {
    if (input.boneIds[i] >= 0 && input.boneIds[i] < MAX_BONES && input.weights[i] > 0.0f)
    {
      skin += Bones[input.boneIds[i]] * input.weights[i];
      totalWeight += input.weights[i];
    }
  }
  return totalWeight > 0.00001f ? skin / totalWeight
    : float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
}

PSInput VSMain(VSInput input)
{
  const float4x4 skin = SkinMatrix(input);
  const float4 worldPosition = mul(Model, mul(skin, float4(input.position, 1.0f)));
  const float3x3 linearTransform = mul((float3x3)Model, (float3x3)skin);
  const float3 normal = normalize(mul(linearTransform, input.normal));
  float3 tangent = mul(linearTransform, input.tangent);
  tangent = normalize(tangent - normal * dot(normal, tangent));
  const float handedness = dot(cross(input.normal, input.tangent), input.bitangent) < 0.0f ? -1.0f : 1.0f;
  PSInput output;
  float4 clipPosition = mul(ViewProjection, worldPosition);
  const float2 outputResolution = max(Resolution.xy, float2(1.0f, 1.0f));
  const float virtualHeight = max(Resolution.z, 1.0f);
  const float2 snapResolution = float2(
    max(floor(virtualHeight * outputResolution.x / outputResolution.y + 0.5f), 1.0f),
    virtualHeight);
  const float safeW = abs(clipPosition.w) > 0.00001f ? clipPosition.w : 0.00001f;
  if (Resolution.w > 0.5f)
  {
    const float2 ndc = clipPosition.xy / safeW;
    const float2 snappedNdc =
      (floor((ndc * 0.5f + 0.5f) * snapResolution + 0.5f) / snapResolution) * 2.0f - 1.0f;
    clipPosition.xy = snappedNdc * clipPosition.w;
  }
  output.position = clipPosition;
  output.worldPosition = worldPosition.xyz;
  output.normal = normal;
  output.tangent = tangent;
  output.bitangent = normalize(cross(normal, tangent)) * handedness;
  output.uv = input.uv;
  output.shadowPosition = mul(LightViewProjection, worldPosition);
  return output;
}

float ShadowFactor(float4 shadowPosition, float3 normal, float3 lightDirection)
{
  if (LightCount.y == 0) return 1.0f;
  if (shadowPosition.w <= 0.0f) return 1.0f;
  const float3 ndc = shadowPosition.xyz / shadowPosition.w;
  const float2 uv = ndc.xy * float2(0.5f, -0.5f) + 0.5f;
  if (ndc.z <= 0.0f || ndc.z >= 1.0f || any(uv < 0.0f) || any(uv > 1.0f)) return 1.0f;
  uint width, height;
  ShadowTexture.GetDimensions(width, height);
  const float2 texel = 1.0f / float2(width, height);
  const float bias = max(0.0015f * (1.0f - saturate(dot(normal, -lightDirection))), 0.0002f);
  float visibility = 0.0f;
  [unroll]
  for (int y = -1; y <= 1; ++y)
    [unroll]
    for (int x = -1; x <= 1; ++x)
    {
      const float depth = ShadowTexture.SampleLevel(LinearSampler, uv + float2(x, y) * texel, 0).r;
      visibility += ndc.z - bias <= depth ? 1.0f : 0.18f;
    }
  return visibility / 9.0f;
}

float PointShadowFactor(float3 worldPosition, float3 normal, float3 lightPosition,
                        float shadowSlotPlusOne)
{
  if (LightCount.y == 0 || shadowSlotPlusOne < 0.5f) return 1.0f;
  const float3 fragToLight = worldPosition - lightPosition;
  const float currentDepth = length(fragToLight);
  if (currentDepth >= 20.0f || currentDepth <= 0.0001f) return 1.0f;
  const float3 direction = fragToLight / currentDepth;
  const float closestDepth = PointShadowTexture.SampleLevel(
    LinearSampler, float4(direction, shadowSlotPlusOne - 1.0f), 0).r;
  const float bias = max(0.03f * (1.0f - dot(normal, -direction)), 0.003f);
  return currentDepth - bias > closestDepth ? 0.15f : 1.0f;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  const float4 albedoSample = DiffuseTexture.Sample(LinearSampler, input.uv);
  float3 normal = normalize(input.normal);
  if (MaterialFlags.x > 0.5f)
  {
    const float3 tangentNormal = NormalTexture.Sample(LinearSampler, input.uv).xyz * 2.0f - 1.0f;
    normal = normalize(tangentNormal.x * input.tangent + tangentNormal.y * input.bitangent + tangentNormal.z * normal);
  }
  const float specularStrength = MaterialFlags.y > 0.5f
    ? SpecularTexture.Sample(LinearSampler, input.uv).r : 0.12f;
  const float3 viewDirection = normalize(CameraPosition.xyz - input.worldPosition);
  const float shininess = lerp(8.0f, 128.0f, specularStrength);
  float3 lighting = 0.0f;
  [loop]
  for (uint i = 0; i < min(LightCount.x, (uint)MAX_LIGHTS); ++i)
  {
    const int type = (int)(LightTypes[i].x + 0.5f);
    const float3 lightColor = LightColors[i].rgb;
    const float3 ambient = lightColor * 0.1f;
    float3 toLight;
    float attenuation = 1.0f;
    float intensity = 1.0f;
    float shadow = 1.0f;
    if (type == 0)
    {
      const float3 direction = normalize(LightDirections[i].xyz);
      toLight = -direction;
      shadow = ShadowFactor(input.shadowPosition, normal, direction);
      const float diffuse = saturate(dot(normal, toLight));
      const float specular = pow(saturate(dot(viewDirection, reflect(-toLight, normal))), shininess);
      lighting += ambient * albedoSample.rgb;
      lighting += lightColor * diffuse * albedoSample.rgb * shadow;
      lighting += specularStrength * specular * shadow;
    }
    else if (type == 1)
    {
      const float3 offset = LightPositions[i].xyz - input.worldPosition;
      const float distanceToLight = max(length(offset), 0.001f);
      toLight = offset / distanceToLight;
      attenuation = 1.0f / (1.0f + 0.09f * distanceToLight + 0.032f * distanceToLight * distanceToLight);
      const float diffuse = saturate(dot(normal, toLight));
      const float specular = pow(saturate(dot(viewDirection, reflect(-toLight, normal))), shininess);
      shadow = PointShadowFactor(input.worldPosition, normal, LightPositions[i].xyz, LightTypes[i].y);
      lighting += ambient * albedoSample.rgb * attenuation;
      lighting += lightColor * diffuse * albedoSample.rgb * attenuation * shadow;
      lighting += specularStrength * specular * attenuation * shadow;
    }
    else
    {
      const float3 offset = LightPositions[i].xyz - input.worldPosition;
      const float distanceToLight = max(length(offset), 0.001f);
      toLight = offset / distanceToLight;
      attenuation = 1.0f / (1.0f + 0.09f * distanceToLight + 0.032f * distanceToLight * distanceToLight);
      const float theta = dot(-toLight, normalize(LightDirections[i].xyz));
      intensity = saturate((theta - cos(0.3054f)) / max(cos(0.2182f) - cos(0.3054f), 0.0001f));
      const float diffuse = saturate(dot(normal, toLight));
      const float specular = pow(saturate(dot(viewDirection, reflect(-toLight, normal))), shininess);
      lighting += (ambient * albedoSample.rgb + lightColor * diffuse * albedoSample.rgb
        + specularStrength * specular) * attenuation * intensity;
    }
  }
  return float4(lighting, albedoSample.a);
}