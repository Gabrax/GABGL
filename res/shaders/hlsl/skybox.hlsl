cbuffer SkyboxConstants : register(b0) { float4x4 ViewProjection; };
TextureCube SkyboxTexture : register(t0);
SamplerState SkyboxSampler : register(s0);
struct PSInput { float4 position : SV_POSITION; float3 direction : TEXCOORD; };
PSInput VSMain(float3 position : POSITION)
{
  PSInput output;
  float4 clip = mul(ViewProjection, float4(position, 1.0f));
  output.position = clip.xyww;
  output.direction = position;
  return output;
}
float4 PSMain(PSInput input) : SV_TARGET
{
  return float4(SkyboxTexture.Sample(SkyboxSampler, input.direction).rgb * 0.65f, 1.0f);
}