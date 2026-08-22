cbuffer UIConstants : register(b0)
{
  float2 ScreenSize;
};

Texture2D FontAtlas : register(t0);
SamplerState FontSampler : register(s0);

struct VSInput
{
  float2 position : POSITION;
  float4 color : COLOR;
  float2 uv : TEXCOORD;
};

struct PSInput
{
  float4 position : SV_POSITION;
  float4 color : COLOR;
  float2 uv : TEXCOORD;
};

PSInput VSMain(VSInput input)
{
  PSInput output;
  const float2 safeSize = max(ScreenSize, float2(1.0f, 1.0f));
  output.position = float4(input.position.x / safeSize.x * 2.0f - 1.0f,
                           input.position.y / safeSize.y * 2.0f - 1.0f,
                           0.0f, 1.0f);
  output.color = input.color;
  output.uv = input.uv;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  const float coverage = FontAtlas.Sample(FontSampler, input.uv).r;
  return float4(input.color.rgb, input.color.a * coverage);
}