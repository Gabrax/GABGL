#define MAX_BONES 100
cbuffer PointShadowConstants : register(b0)
{
  float4x4 LightViewProjection;
  float4x4 Model;
  float4x4 Bones[MAX_BONES];
  float4 LightPositionAndAnimated;
};
struct VSInput { float3 position : POSITION; int4 boneIds : BONEIDS; float4 weights : BONEWEIGHTS; };
struct PSInput { float4 position : SV_POSITION; float3 worldPosition : POSITION0; };
PSInput VSMain(VSInput input)
{
  float4x4 skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  if (LightPositionAndAnimated.w > 0.5f)
  {
    skin = float4x4(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0);
    float total = 0.0f;
    [unroll] for (int i = 0; i < 4; ++i)
      if (input.boneIds[i] >= 0 && input.boneIds[i] < MAX_BONES && input.weights[i] > 0.0f)
      { skin += Bones[input.boneIds[i]] * input.weights[i]; total += input.weights[i]; }
    if (total > 0.00001f) skin /= total;
    else skin = float4x4(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
  }
  const float4 world = mul(Model, mul(skin, float4(input.position, 1.0f)));
  PSInput output;
  output.position = mul(LightViewProjection, world);
  output.worldPosition = world.xyz;
  return output;
}
float PSMain(PSInput input) : SV_TARGET
{
  return length(input.worldPosition - LightPositionAndAnimated.xyz);
}